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

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "ipc_def.h"

int main(void)
{
    cy_rslt_t result;

    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    __enable_irq();

    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, NULL);

    Cy_SCB_UART_Enable(CYBSP_UART_HW);

    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\x1b[2J\x1b[;H");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "===== Cyberon Keyword Detection Engine Demo =====\r\n");

    Cy_IPC_Sema_Clear(SEMA_NUM, false);
    __SEV();

    while(1)
    {
        Cy_SysPm_CpuEnterSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
    }
}

