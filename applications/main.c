#include <rtthread.h>
#include <rtdevice.h>
#include "wiota_app.h"
#include "uc_example_app.h"
#include "uc_wiota_static.h"

#ifdef RT_TASK_RESOURCE_TOOL
#include "resource_manager.h"
#endif


static int memory_show(void)
{
    rt_size_t total;
    rt_size_t used;
    rt_size_t max_used;

    rt_memory_info(&total, &used, &max_used);
    rt_kprintf("total %d used %d maxused %d\n", total, used, max_used);

    return 0;
}

int main(void)
{
    uc_wiota_static_data_init(); // must call and place first!!!

    uc_peripheral_example();
    wiota_app_init();

#ifdef RT_TASK_RESOURCE_TOOL
    resource_manager_init();
#endif

    while (1)
    {
        rt_thread_mdelay(7000);
        memory_show();
#ifdef RT_TASK_RESOURCE_TOOL
        // resource_manager(RESOURCE_SIMPLE_MODE);
#endif
    }

    return 0;
}
