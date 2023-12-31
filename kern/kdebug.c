#include <inc/stab.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>

#include <kern/kdebug.h>

extern const struct Stab __STAB_BEGIN__[]; // Beginning of stabs table
extern const struct Stab __STAB_END__[];   // End of stabs table
extern const char __STABSTR_BEGIN__[];     // Beginning of string table
extern const char __STABSTR_END__[];       // End of string table


void show_stab_info(int start, int end, int set)
{
    static int set_mask = SO | SOL | FUN | PSYM | SLINE;
    static int map[] =
        {
            [N_FUN] = FUN,
            [N_SLINE] = SLINE,
            [N_SO] = SO,
            [N_SOL] = SOL,
            [N_PSYM] = PSYM,
        };
    static const char *type_str[] =
        {
            [N_FUN] = "FUN",
            [N_SLINE] = "SLINE",
            [N_SO] = "SO",
            [N_SOL] = "SOL",
            [N_PSYM] = "PSYM",
        };

    set &= set_mask;
    const char *stabstr = __STABSTR_BEGIN__;
    uint32_t preaddr = 0;
    for (int j = start; j <= end + 1; ++j)
    {
        struct Stab *ps = (struct Stab *)&__STAB_BEGIN__[j];
        if (ps->n_type >= 0xa0)
            continue;
        if (map[ps->n_type] & set)
        {
            if (ps->n_type != N_SLINE)
            {
                if (ps->n_value <= preaddr)
                {
                    cprintf("fuck: %d %08x\n", j,  ps->n_value);

                    assert(false);
                }
                preaddr = ps->n_type;
            }
            cprintf("%d %s %s %08x\n", j, stabstr + ps->n_strx, type_str[ps->n_type], ps->n_value);
        }
    }
}

// stab_binsearch(stabs, region_left, region_right, type, addr)
//
//	Some stab types are arranged in increasing order by instruction
//	address.  For example, N_FUN stabs (stab entries with n_type ==
//	N_FUN), which mark functions, and N_SO stabs, which mark source files.
//
//	Given an instruction address, this function finds the single stab
//	entry of type 'type' that contains that address.
//
//	The search takes place within the range [*region_left, *region_right].
//	Thus, to search an entire set of N stabs, you might do:
//
//		left = 0;
//		right = N - 1;     /* rightmost stab */
//		stab_binsearch(stabs, &left, &right, type, addr);
//
//	The search modifies *region_left and *region_right to bracket the
//	'addr'.  *region_left points to the matching stab that contains
//	'addr', and *region_right points just before the next stab.  If
//	*region_left > *region_right, then 'addr' is not contained in any
//	matching stab.
//
//	For example, given these N_SO stabs:
//		Index  Type   Address
//		0      SO     f0100000
//		13     SO     f0100040
//		117    SO     f0100176
//		118    SO     f0100178
//		555    SO     f0100652
//		556    SO     f0100654
//		657    SO     f0100849
//	this code:
//		left = 0, right = 657;
//		stab_binsearch(stabs, &left, &right, N_SO, 0xf0100184);
//	will exit setting left = 118, right = 554.
//
static void
stab_binsearch(const struct Stab *stabs, int *region_left, int *region_right,
               int type, uintptr_t addr)
{
    int l = *region_left, r = *region_right, any_matches = 0;

    while (l <= r)
    {
        int true_m = (l + r) / 2, m = true_m;

        // search for earliest stab with right type
        while (m >= l && stabs[m].n_type != type)
            m--;
        if (m < l)
        { // no match in [l, m]
            l = true_m + 1;
            continue;
        }

        // actual binary search
        any_matches = 1;
        if (stabs[m].n_value < addr)
        {
            *region_left = m;
            l = true_m + 1;
        }
        else if (stabs[m].n_value > addr)
        {
            *region_right = m - 1;
            r = m - 1;
        }
        else
        {
            // exact match for 'addr', but continue loop to find
            // *region_right
            cprintf("fuck equal\n");
            *region_left = m;
            l = m;
            addr++;
        }
    }

    if (!any_matches)
        *region_right = *region_left - 1;
    else
    {
        // find rightmost region containing 'addr'
        for (l = *region_right;
             l > *region_left && stabs[l].n_type != type;
             l--)
            /* do nothing */;
        if (*region_left != l)
        {
            cprintf("fuck change\n");

            *region_left = l;
        }
    }
}

// debuginfo_eip(addr, info)
//
//	Fill in the 'info' structure with information about the specified
//	instruction address, 'addr'.  Returns 0 if information was found, and
//	negative if not.  But even if it returns negative it has stored some
//	information into '*info'.
//
int debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
{
    const struct Stab *stabs, *stab_end;
    const char *stabstr, *stabstr_end;
    int lfile, rfile, lfun, rfun, lline, rline;

    // Initialize *info
    info->eip_file = "<unknown>";
    info->eip_line = 0;
    info->eip_fn_name = "<unknown>";
    info->eip_fn_namelen = 9;
    info->eip_fn_addr = addr;
    info->eip_fn_narg = 0;

    // Find the relevant set of stabs
    if (addr >= ULIM)
    {
        stabs = __STAB_BEGIN__;
        stab_end = __STAB_END__;
        stabstr = __STABSTR_BEGIN__;
        stabstr_end = __STABSTR_END__;
    }
    else
    {
        // Can't search for user-level addresses yet!
        panic("User address");
    }

    // String table validity checks
    if (stabstr_end <= stabstr || stabstr_end[-1] != 0)
        return -1;

    // Now we find the right stabs that define the function containing
    // 'eip'.  First, we find the basic source file containing 'eip'.
    // Then, we look in that source file for the function.  Then we look
    // for the line number.

    // Search the entire set of stabs for the source file (type N_SO).
    lfile = 0;
    rfile = (stab_end - stabs) - 1;
    cprintf("stat %d end %d\n", lfile, rfile);
    stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
    if (lfile == 0)
        return -1;
    cprintf("======FILE=========\n");
    cprintf("%d - %d %x\n", lfile, rfile, addr);
    show_stab_info(lfile, rfile, SO | SOL | FUN);
    cprintf("==================\n");
    // Search within that file's stabs for the function definition
    // (N_FUN).
    lfun = lfile;
    rfun = rfile;
    stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);

    if (lfun <= rfun)
    {
        cprintf("======FUN=========\n");
        cprintf("%d - %d %x\n", lfun, rfun, addr);
        show_stab_info(lfun, rfun, FUN);
        cprintf("==================\n");
        // stabs[lfun] points to the function name
        // in the string table, but check bounds just in case.
        if (stabs[lfun].n_strx < stabstr_end - stabstr)
            info->eip_fn_name = stabstr + stabs[lfun].n_strx;
        info->eip_fn_addr = stabs[lfun].n_value;
        addr -= info->eip_fn_addr;
        // Search within the function definition for the line number.
        lline = lfun;
        rline = rfun;
    }
    else
    {
        // Couldn't find function stab!  Maybe we're in an assembly
        // file.  Search the whole file for the line number.
        info->eip_fn_addr = addr;
        lline = lfile;
        rline = rfile;
    }
    // Ignore stuff after the colon.
    info->eip_fn_namelen = strfind(info->eip_fn_name, ':') - info->eip_fn_name;

    // Search within [lline, rline] for the line number stab.
    // If found, set info->eip_line to the right line number.
    // If not found, return -1.
    //
    // Hint:
    //	There's a particular stabs type used for line numbers.
    //	Look at the STABS documentation and <inc/stab.h> to find
    //	which one.
    // Your code here.
    stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
    cprintf("======LINE=========\n");
    cprintf("%d - %d %x\n", lline, rline, addr);
    show_stab_info(lline, rline, FUN | SLINE);
    cprintf("==================\n");
    if (lline > rline)
        return -1;
    info->eip_line = lline;

    // Search backwards from the line number for the relevant filename
    // stab.
    // We can't just use the "lfile" stab because inlined functions
    // can interpolate code from a different file!
    // Such included source files use the N_SOL stab type.
    while (lline >= lfile && stabs[lline].n_type != N_SOL && (stabs[lline].n_type != N_SO || !stabs[lline].n_value))
        lline--;
    if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr)
        info->eip_file = stabstr + stabs[lline].n_strx;

    // Set eip_fn_narg to the number of arguments taken by the function,
    // or 0 if there was no containing function.
    if (lfun < rfun)
        for (lline = lfun + 1;
             lline < rfun && stabs[lline].n_type == N_PSYM;
             lline++)
            info->eip_fn_narg++;
    cprintf("\n");
    return 0;
}
