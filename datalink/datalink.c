#pragma comment(lib,"ws2_32.lib")
#include <stdio.h>
#include <string.h>
#include <concurrencysal.h>
#include<corecrt.h>
#include<corecrt_memcpy_s.h>
#include<corecrt_memory.h>
#include<corecrt_stdio_config.h>
#include<corecrt_wstdio.h>
#include<corecrt_wstring.h>
#include<errno.h>
#include<sal.h>
//#include<sourceannotations.h>
#include<vadefs.h>
#include<vcruntime.h>
#include<vcruntime_string.h>
#include<winapifamily.h>
#include<winpackagefamily.h>
//#include<sourceannotations.h>
#include "protocol.h"

#define MAX_SEQ 15                  //���Ĵ��ڴ�С
#define NR_BUFS ((MAX_SEQ + 1) / 2)     //��������С

#define DATA_TIMER  3000       //֡��ʱʱ����
#define ACK_TIMER 240          //ack�ĳ�ʱ���

struct FRAME { 
	//����֡�Ľṹ�嶨��
    unsigned char kind;
    unsigned char ack;
    unsigned char seq;
    unsigned char data[PKT_LEN]; 
    unsigned int  padding;
};

int no_nak=1;							//��־�Ƿ��Ѿ����͹�nak
static int phl_ready = 0;				//�����������
unsigned char oldest_frame = MAX_SEQ + 1;	//

static int between(unsigned char a,unsigned char b,unsigned char c)  
//���������жϺ������ж�֡�ţ�ack�ţ��Ƿ��ڴ�����
{
   if(((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
		return 1;
	else
		return 0;

}

static void put_frame(unsigned char *frame, int len)
{
	//����֡ǰԤ��������У���crc
    *(unsigned int *)(frame + len) = crc32(frame, len);//crc32У��
    send_frame(frame, len + 4);//����֡
    phl_ready = 0;//�������������
}

static void send_data_frame(unsigned char fk,unsigned char frame_nr,unsigned char frame_expected,unsigned char buffer[NR_BUFS][PKT_LEN])
{
	//��������֡����ack��nak
    struct FRAME s;
    
    s.kind = fk;
    s.seq = frame_nr;
    s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);

	if(fk==FRAME_DATA)//���͵�������֡
	{
		memcpy(s.data, buffer[frame_nr % NR_BUFS], PKT_LEN);//���Ʒ��鵽֡��
		dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);//�����¼
        put_frame((unsigned char *)&s, 3 + PKT_LEN);   //����
		start_timer(frame_nr % NR_BUFS, DATA_TIMER);     //������ʱ��
	}
	else if(fk == FRAME_NAK)//���͵���NAK
	{
		no_nak = 0;                                     //��һ�β��ٷ���nak
	    put_frame((unsigned char *)&s, 3);            //����
	}
	else if(fk == FRAME_ACK)//���͵���ACK
	{
		dbg_frame("Send ACK  %d\n", s.ack);//�����¼
        put_frame((unsigned char *)&s, 3);//����
	}
	phl_ready = 0;							//���������
	stop_ack_timer();                       //�ر�ack��ʱ��
}

void main(int argc, char **argv)
{
	int event, arg;
    struct FRAME f;
    int len = 0;
    int i;
	static unsigned char ack_expected = 0, next_frame_to_send = 0;//��ǰ֡����һ����֡�����
	static unsigned char frame_expected = 0, too_far = NR_BUFS;
    static unsigned char nbuffered;
	int arrived[NR_BUFS];//����������ŵ����֡
	static unsigned char out_buf[NR_BUFS][PKT_LEN], in_buf[NR_BUFS][PKT_LEN];

    protocol_init(argc, argv);//��ʼ��Э��

    lprintf("Coded by zdf, Build Time: " __DATE__"  "__TIME__"\n");

	for(i = 0; i < NR_BUFS; i++)   //�����շ��Ļ��������
		arrived[i] = 0;

    enable_network_layer();//��������㣬׼����������

	while(1)
	{
		event = wait_for_event(&arg);			//�ȴ��¼���������һ������
		switch (event)
		{
			case NETWORK_LAYER_READY:
				nbuffered++;                    //����ȴδ��ȷ�ϵ�֡������+1
				get_packet(out_buf[next_frame_to_send % NR_BUFS]);//�õ�������뻺������
				send_data_frame(FRAME_DATA,next_frame_to_send,frame_expected,out_buf);//��������֡
				next_frame_to_send=(next_frame_to_send + 1) % ( MAX_SEQ + 1);//����������
				break;

			case PHYSICAL_LAYER_READY:
				phl_ready = 1;
				break;

			case FRAME_RECEIVED:
				len = recv_frame((unsigned char *)&f, sizeof f);
                if (len < 5 || crc32((unsigned char *)&f, len) != 0)
				{	//У��ʹ��󣬷���NAK�����ش�
				   	if(no_nak == 1)
				   	    send_data_frame(FRAME_NAK, 0, frame_expected, out_buf);
                    dbg_event("---- Receive Frame Error, Bad CRC Checksum ----\n");//�����¼
                    break;
				}
				if (f.kind == FRAME_DATA)
			    {
					//�յ���������֡
					if((f.seq != frame_expected) && no_nak == 1)// ֡���кŴ��󣬲�����
						send_data_frame(FRAME_NAK,0, frame_expected, out_buf);//����NAK
					else 
						start_ack_timer(ACK_TIMER);//����ack��ʱ�����Ӵ�ȷ�ϣ�
					if(between(frame_expected, f.seq, too_far) == 1 && arrived[f.seq % NR_BUFS] == 0)
					{
						//���յ���֡�ڽ��ܷ�������������д���
						dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);//�����¼
						arrived[f.seq % NR_BUFS] = 1;//��¼�յ���֡
						memcpy(in_buf[f.seq % NR_BUFS], f.data, len - MAX_SEQ / 2);
						while(arrived[frame_expected % NR_BUFS])
						{
							//һ�δ��������ڲ�������֡����С�ڵ�ǰ���кŵ�֡˳��ת���������
							put_packet(in_buf[frame_expected % NR_BUFS], len - MAX_SEQ / 2);
							no_nak = 1;//��λNAK���
							arrived[frame_expected%NR_BUFS] = 0;//��ջ�������־

							frame_expected = (frame_expected+1) % (MAX_SEQ + 1);//����ָ�븴λ
							too_far = (too_far + 1) % (MAX_SEQ + 1);//���շ��������һ���1��λ
							start_ack_timer(ACK_TIMER);//����ack��ʱ��
						}
					}
			    }
				if((f.kind == FRAME_NAK) && between(ack_expected,(f.ack + 1) % (MAX_SEQ + 1), next_frame_to_send) == 1)
				{
					//���ͷ��յ�nak,�ش���ʧ��������֡
					dbg_frame("Recv NAK %d\n", f.ack);//�����¼
					send_data_frame(FRAME_DATA,(f.ack + 1) % (MAX_SEQ + 1), frame_expected, out_buf);
				}
				while(between(ack_expected,f.ack,next_frame_to_send) == 1)
				{
					//�յ�����֡���������
					nbuffered--;		//����ȴδ��ȷ�ϵ�֡������-1
					stop_timer(ack_expected % NR_BUFS);			//�ر�֡�ļ�ʱ��
					ack_expected = (ack_expected + 1) % (MAX_SEQ + 1);
				}
				break;

			case DATA_TIMEOUT:
				//���ݶ�ʧ�����·�������֡
                dbg_event("---- DATA %d timeout\n", arg);		//�����¼
                send_data_frame(FRAME_DATA,ack_expected, frame_expected, out_buf);//���泬ʱ��Ҫ���ش�����
                break;
			case ACK_TIMEOUT:
				//����ACK��ʧ����������ACKȷ��֡
				dbg_event("---- ACK %d timeout\n", arg);		//�����¼
				send_data_frame(FRAME_ACK, 0, frame_expected, out_buf);//���泬ʱ��Ҫ���ش�ACK
		}

        if (nbuffered < NR_BUFS && phl_ready)
			//������û������������~
            enable_network_layer();
		else
			//��������ը����ͣ���ݴ���
            disable_network_layer();
	}
}
