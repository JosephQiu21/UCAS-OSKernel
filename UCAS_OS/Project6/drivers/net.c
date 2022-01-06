#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode;

volatile int rx_curr = 0, rx_tail = 0;

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?
    EmacPsSetupBD(&EmacPsInstance);
    int num_packet_left = num_packet;
    while (num_packet_left > 0) {
        int num_recv = (num_packet_left > 32) ? 32 : num_packet_left;
        EmacPsRecv(&EmacPsInstance, rx_buffers, num_recv);
        EmacPsWaitRecv(&EmacPsInstance, num_recv, rx_len);
        for (int i = 0; i < num_recv; i++) {
            kmemcpy(addr, rx_buffers[i], rx_len[i]);
            *frLength = rx_len[i];
            frLength++;
            addr += rx_len[i];
        }
        num_packet_left -= 32;
        
    }
    return 1;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // send all packet
    // maybe you need to call drivers' send function multiple times ?
    kmemcpy((uint8_t *)tx_buffer, (uint8_t *)addr, length);
    EmacPsSend(&EmacPsInstance, tx_buffer, length);
    EmacPsWaitSend(&EmacPsInstance);
}

void do_net_irq_mode(int mode)
{
    if (mode) {
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_IER_OFFSET, XEMACPS_IXR_ALL_MASK);
    }
    net_poll_mode = mode;
}
