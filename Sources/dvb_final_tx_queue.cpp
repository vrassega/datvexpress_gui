#include <queue>
#include <deque>
#include <list>
#include <queue>
#include <semaphore.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include "dvb_uhd.h"
#include <queue>
#include <deque>
#include <list>
#include "dvb.h"
#include "dvb_gen.h"
#include "dvb_config.h"
#include "tx_hardware.h"
#include "dvb_buffer.h"

using namespace std;

static sem_t f_txq_sem;

static queue <dvb_buffer *> m_tx_q;

//
// This is the time delay through the system
// It can be used to calculate the delay and
// time offsets.
//
double final_txq_time_delay( void )
{
    double rate,fdelay;
    // Get the un interpolated sample clock rate
    rate = hw_uniterpolated_sample_rate();
    // Get the size of the unsent USB data in IQ samples
    double qout = hw_outstanding_queue_size();
    // Get the number of outstanding samples on the transmit queue
    double fq_size = m_tx_q.size();
    // Calculate the total number of outstanding samples
    fq_size        = fq_size * TX_BUFFER_LENGTH;
    fq_size       += qout;
    // Calculate the resultant time delay
    fdelay = fq_size / rate;
    return fdelay;
}
//
// Read percentage full of final queue, no semaphore protection
//
int final_tx_queue_percentage_unprotected( void )
{
    // return as a percentage
    int ival = (m_tx_q.size()*100)/N_TX_BUFFS;
    return (ival);
}
//
// Read the fullness of the final queue with semaphore protection
//
int final_tx_queue_size( void )
{
    // return as a percentage
    sem_wait( &f_txq_sem );
    int ival = m_tx_q.size();
    sem_post( &f_txq_sem );
    return (ival);
}
//
// Write samples to the final TX queue
//
void write_final_tx_queue( scmplx* samples, int length )
{
    sem_wait( &f_txq_sem );
    int i;

    if( m_tx_q.size() < N_TX_BUFFS)
    {
        for( i = 0; i < (length - TX_BUFFER_LENGTH); i += TX_BUFFER_LENGTH )
        {
            dvb_buffer *b = dvb_buffer_alloc( TX_BUFFER_LENGTH, BUF_SCMPLX );
            dvb_buffer_write( b, &samples[i] );
            m_tx_q.push( b );
        }
        length = length - i;
        if(length > 0 )
        {
            dvb_buffer *b = dvb_buffer_alloc( length, BUF_SCMPLX );
            dvb_buffer_write( b, &samples[i] );
            m_tx_q.push( b );
        }

    }
    sem_post( &f_txq_sem );
}
void write_final_tx_queue_ts( uchar* tp )
{
    sem_wait( &f_txq_sem );

    if( m_tx_q.size() < N_TX_BUFFS)
    {
        dvb_buffer *b = dvb_buffer_alloc( 188, BUF_TS );
        dvb_buffer_write( b, tp );
        m_tx_q.push( b );
    }

    sem_post( &f_txq_sem );
}
void write_final_tx_queue_udp( uchar* tp )
{
    sem_wait( &f_txq_sem );

    if( m_tx_q.size() < N_TX_BUFFS)
    {
        dvb_buffer *b = dvb_buffer_alloc( 188, BUF_UDP );
        dvb_buffer_write( b, tp );
        m_tx_q.push( b );
    }

    sem_post( &f_txq_sem );
}
//
// Read from the final TX queue
//
dvb_buffer *read_final_tx_queue(void)
{
    dvb_buffer *b;

    sem_wait( &f_txq_sem );

    if( m_tx_q.size() > 0 )
    {
        b = m_tx_q.front();
        m_tx_q.pop();
    }
    else
    {
        b      = NULL;
        dvb_block_rx_check();
    }
    sem_post( &f_txq_sem );
    return b;
}
void create_final_tx_queue( void )
{
    sem_init( &f_txq_sem, 0, 0 );
    sem_post( &f_txq_sem );
}