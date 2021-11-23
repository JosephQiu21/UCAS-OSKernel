#include <mailbox.h>
#include <string.h>

mailbox_t mbox_open(char *name)
{
    return sys_mbox_open(name);
}

void mbox_close(mailbox_t mailbox)
{
    sys_mbox_close(mailbox);
}

int mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    return sys_mbox_send(mailbox, msg, msg_length);
}

int mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    return sys_mbox_recv(mailbox, msg, msg_length);
}
