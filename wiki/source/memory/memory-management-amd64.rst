Memory Management in LM64
=========================


This is how paging works in LM64 (called IA-32e by Intel):

The 64-bit paging is enabled by having the following
bits set to 1:

   * CR0.PE(0) - Protection Enable.
   * CR0.PG(31) - Paging.
   * CR4.PAE(5) - Physical Address Extension.
   * EFER.LME(7) - Long-Mode Enable.


The operation is controlled by the following registers:

   * CR2 - Page-Fault Linear Address.
   * CR3 - Page Directory Base Register.


The flow is as follows:

   #. CR3 indicates where to find the Page Map Level 4, which is nothing more
      than a table containing addresses of Page Directory Pointer tables.

   #. Each PDP table contains entries, each of which can point to a Page
      Directory table, or map a virtual memory page of 1 GiB size.

   #. Each Page Directory itself is a table whose entries can either point
      to Page Tables, or map 2 MiB virtual memory pages.

   #. Each one of the Page Table entries contains a mapping to correlate
      a 4 KiB virtual memory page to its location in physical memory.


**Virtual memory resolution with 4 KiB pages**

When a virtual memory address comes to the CPU for decoding, the PML4 is
indexed with bits 39-47, then the PDPT to which that entry is indexed
with bits 30-38, from which a PD is selected, then bits 21-29 are used
to index it to select the PT, which is indexed by bits 12-20 to find
the requested page.  This gives us the physical address of this 4 KiB
region where we can finally access the data we want by adding the offset
from bits 0-11.

**Virtual memory resolution with 2 MiB pages**

It works similarly to resolving using 4 KiB pages, except there is no Page
Table in the end, so the nine bits that would be used to index it are instead
used as most-significant bits of the (now 21-bits long) offset, and the PD
entry points directly to the physical addres of the page.

**Virtual memory resolution with 1 GiB pages**

This mechanism allows to remove yet another layer of indirection, by having
the PDPT entry point directy to the physical page.  In this case, the offset
is extended to take the 30 bits that would otherwise be used to index the Page
Directory and Page Table.

.. code::

      +---------+---------+---------+---------+------------+
      | PML4 idx| PDPT idx| PD Index| PT Index|   Offset   |
      +---------+---------+---------+---------+------------+
      47       39        30        21        12            0
     
      +---------+---------+---------+----------------------+
      | PML4 idx| PDPT idx| PD Index| -----  Offset -----  |
      +---------+---------+---------+----------------------+
      47       39        30        21                      0
     
      +---------+---------+--------------------------------+
      | PML4 idx| PDPT idx| ---------   Offset   --------- |
      +---------+---------+--------------------------------+
      47       39        30                                0

That said, in order to create a new Page Table entry, we need to ensure
the full plumbing exists, so we must make sure we have all of the space
needed to allocate these tables.  There are two options: set aside a chunk
of physical memory fo all of the tables you will ever need, or allocate them
dynamically as they are required.

For this exercise, we will assume we have at least 2 MiB of physical RAM.
We will create a single 2 MiB page, identity-mapped at the beginning of both
addres spaces, with full permissions for read, write and execute.  it will be
created in PM32 mode, and it will be conserved in LM64 throughout the lifetime
of the system.  This page will be reserved for system-level data structures,
and no code, --kernel or user-- will be executed from it after the system
has bootstrapped.

We will assume that 1 GiB is plenty for the boot process and allocate the 4096
bytes for a fully-populated Page Directory, which originally will only contain
the basic identity-mapped 2 MiB page at 0.  More entries will be added to it
as needed and, should we ever need another Page Directory, it is not terribly
difficult to add it at that time.  This will only take care of System pages,
meaning those pages nneded by the boot sector, Start, and the Kernel Four (Elfo,
Memo, Anjo and IOIO).

