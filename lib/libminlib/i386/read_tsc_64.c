
#include <minix/u64.h>
#include <minix/minlib.h>
#include <minix/cpufeature.h>

/* Utility function to work directly with u64_t
 * By Antonio Mancina
 */
void read_tsc_64(t)
u64_t* t;
{
    static int cpu_has_tsc = -1;

    if (cpu_has_tsc == -1)
	cpu_has_tsc = _cpufeature(_CPUF_I386_TSC);
    if (!cpu_has_tsc)
        panic("No TSC!");

    u32_t lo, hi;
    read_tsc (&hi, &lo);
    *t = make64 (lo, hi);
}

