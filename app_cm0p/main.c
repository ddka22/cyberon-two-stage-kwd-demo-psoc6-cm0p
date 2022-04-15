/*
 * Copyright 2022 Cyberon Corporation
 *
 * you may not use this file except in compliance with the License.
 * A copy of the license is located in the "LICENSE" file accompanying this source.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdlib.h>
#include <stdio.h>
#include "cy_pdl.h"
#include "ipc_def.h"
#include "cycfg.h"

#include "cyberon_asr.h"

#define FRAME_SIZE                  (480)

void pdm_pcm_isr_handler(void);

void asr_callback(const char *function, char *message, char *parameter);

volatile bool pdm_pcm_flag = false;
int16_t pdm_pcm_ping[FRAME_SIZE] = {0};
int16_t pdm_pcm_pong[FRAME_SIZE] = {0};
int16_t *pdm_pcm_buffer = &pdm_pcm_ping[0];
const cy_stc_sysint_t pdm_dma_int_cfg = {
    .intrSrc = (IRQn_Type) NvicMux7_IRQn,
    .cm0pSrc = CYBSP_PDM_DMA_IRQ,
    .intrPriority = 2
};

int main(void)
{
    char print_message[128];
    uint64_t uid;

    __enable_irq();
    Cy_IPC_Sema_Set(SEMA_NUM, false);
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR);

    do
    {
        __WFE();
    } 
    while(Cy_IPC_Sema_Status(SEMA_NUM) == CY_IPC_SEMA_STATUS_LOCKED);
    
    SystemCoreClockUpdate();

    Cy_DMA_Descriptor_Init(&CYBSP_PDM_DMA_Descriptor_0, &CYBSP_PDM_DMA_Descriptor_0_config);
    Cy_DMA_Descriptor_Init(&CYBSP_PDM_DMA_Descriptor_1, &CYBSP_PDM_DMA_Descriptor_1_config);
    Cy_DMA_Descriptor_SetSrcAddress(&CYBSP_PDM_DMA_Descriptor_0, (const void *) &CYBSP_PDM_PCM_HW->RX_FIFO_RD);
    Cy_DMA_Descriptor_SetSrcAddress(&CYBSP_PDM_DMA_Descriptor_1, (const void *) &CYBSP_PDM_PCM_HW->RX_FIFO_RD);
    Cy_DMA_Descriptor_SetDstAddress(&CYBSP_PDM_DMA_Descriptor_0, (const void *) &pdm_pcm_ping);
    Cy_DMA_Descriptor_SetDstAddress(&CYBSP_PDM_DMA_Descriptor_1, (const void *) &pdm_pcm_pong);
    Cy_DMA_Descriptor_SetXloopDataCount(&CYBSP_PDM_DMA_Descriptor_0, FRAME_SIZE/4);
    Cy_DMA_Descriptor_SetXloopDataCount(&CYBSP_PDM_DMA_Descriptor_1, FRAME_SIZE/4);
    Cy_DMA_Descriptor_SetYloopDstIncrement(&CYBSP_PDM_DMA_Descriptor_0, FRAME_SIZE/4);
    Cy_DMA_Descriptor_SetYloopDstIncrement(&CYBSP_PDM_DMA_Descriptor_1, FRAME_SIZE/4);
    Cy_DMA_Channel_Init(CYBSP_PDM_DMA_HW, CYBSP_PDM_DMA_CHANNEL, &CYBSP_PDM_DMA_channelConfig);
    Cy_DMA_Channel_SetDescriptor(CYBSP_PDM_DMA_HW, CYBSP_PDM_DMA_CHANNEL, &CYBSP_PDM_DMA_Descriptor_0);
    Cy_DMA_Channel_Enable(CYBSP_PDM_DMA_HW, CYBSP_PDM_DMA_CHANNEL);
    Cy_DMA_Enable(CYBSP_PDM_DMA_HW);

    Cy_DMA_Channel_SetInterruptMask(CYBSP_PDM_DMA_HW, CYBSP_PDM_DMA_CHANNEL, CY_DMA_INTR_MASK);
    Cy_SysInt_Init(&pdm_dma_int_cfg, pdm_pcm_isr_handler);
    NVIC_EnableIRQ((IRQn_Type) pdm_dma_int_cfg.intrSrc);

    Cy_PDM_PCM_Init(CYBSP_PDM_PCM_HW, &CYBSP_PDM_PCM_config);
    Cy_PDM_PCM_Enable(CYBSP_PDM_PCM_HW);

    uid = Cy_SysLib_GetUniqueId();
    sprintf(print_message, "uniqueIdHi: 0x%08lX, uniqueIdLo: 0x%08lX\r\n", (uint32_t)(uid >> 32), (uint32_t)(uid << 32 >> 32));
    Cy_SCB_UART_PutString(CYBSP_UART_HW, print_message);

	if(!cyberon_asr_init(asr_callback))
		while(1);

	while(1)
    {
        if(pdm_pcm_flag)
        {
        	pdm_pcm_flag = 0;
            cyberon_asr_process(pdm_pcm_buffer, FRAME_SIZE);
        }
    }
}

void pdm_pcm_isr_handler(void)
{
    static bool ping_pong = false;

    if (ping_pong)
    {
        pdm_pcm_buffer = &pdm_pcm_pong[0];
    }
    else
    { 
        pdm_pcm_buffer = &pdm_pcm_ping[0]; 
    }

    ping_pong = !ping_pong;
    pdm_pcm_flag = true;

    Cy_DMA_Channel_ClearInterrupt(CYBSP_PDM_DMA_HW, CYBSP_PDM_DMA_CHANNEL);
}

void asr_callback(const char *function, char *message, char *parameter)
{
	char print_message[128];

	sprintf(print_message, "[%s]%s(%s)\r\n", function, message, parameter);
	Cy_SCB_UART_PutString(CYBSP_UART_HW, print_message);
}
