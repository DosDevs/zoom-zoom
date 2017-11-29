.. bootsector

The Boot Process
================

.. index:: pair: the; boot process


The Bootsector
--------------

.. index:: pair: the; bootsector

The original approach was to cram everything down in the bootsector
in order to be able to start the second stage already in long mode
and ready to begin initializing the kernel, presumably already loaded
by the 512-byte bootsector.  With all of the challenges this entails,
it was done and it was working at some point.  However, shortly thereafter,
we decided to go in a different direction.

The current purpose of the bootsector is to read the second stage
(called *Start*) and provide functions for finding and reading a file
from the filesystem (since it knows how to do it), in order to avoid
code repetition, even at that small level.


Memory Usage During Boot
------------------------

.. index:: pair: memory; map

The lower few kilobytes of RAM are prepared by the first and second stages
in order to begin booting the kernel.  All of the assembly code and lowest-level
data structures reside here.  It is roughly divided in three parts:

    * The lowest 8KiB contain low-level data structures: 

        - [0000, 0400) The Real-Mode Interrupt Descriptor Table (IDT).
          Right there where the firmware left it.  32-bit pointers
          times 256 entries: 1024 bytes.

        - [0400, 0500) The BIOS Data Area.  We don't touch this.
          We could use it, but we don't, at the moment.  We can even
          reclaim it in 64-bit Long Mode (LM64), once we're sure we won't be
          issuing any BIOS interrupt calls and we're handling all interrupts.

        - [0600, 0800) The bootsector code.  The firmware originally loaded us
          at the inconvenient 7c00h adress, but we relocated here because it's
          further out of the way.

        - [0800, 0A00) The LM64 Tasks State Segment (TSS) I/O Map. It is
          a bitmap where the bit address represents an I/O port and the bit
          value indicates whether the access to the port should be allowed (0)
          or denied (1).  This will be moved to [2000,4000) later on 
          during the start-up process.  65536 ports, one bit per each port:
          8192 byes initialized to all zeroes (curently, 512 byes
          for 4092 ports).  In theory, the first byte after the last port
          should have all eight bits set to one (1).

        - [1000, 2000) The LM64 IDT.  Same number of interrupts as the
          Rea-Mode IDT, but each entry having a 128-bit descriptor: 4096 bytes.

    * The following region, up until the 32KiB mark, is used by the FAT12
      bootsector to store filesystem information to load Start, and kept
      because it's useful during the first stage of Start (in 8086 Real Mode)
      to load the four (? to be determined) basic pillars of the kernel.

        - [2000, 4000) The File Allocation Table (FAT) itself.  In theory,
          we could need up to 6144 bytes for the biggest FAT12, but we all
          know FAT-12 is D-E-D dead, except for playful experiments and
          old equipment simulations.  A full FAT-16 table, more useful but
          only slightly, would need 128 KiB, and a full FAT32 could take
          up to 2048 MiB.  Since the FAT-12 table is so small, we could
          go over the hassle of expanding it to 16-bit entries, in which
          case it would take up to 8192 bytes (4096 per 2 MiB of 512-byte
          cluster storage capacity, 8 MiB of 4 KiB clusters and 16 MiB of
          8 KiB clusters).  But, then again, there is a tradeoff between
          simplifying the reads while complicating the setup, or vice-versa.

        - [4000, 6000) The Root Directory.  These are 32-byte entries in
          FAT filesystems.  Long-name entries take multiple 8.3 entries.
          More information in the FAT section.  Suffice it to say, the static
          FAT-12 and FAT-16 root directories contain usually 224 entries
          (7 KiB) for floppy disks, and 512 (16 KiB) entries for hard-drive
          partitions; but we won't be using typically more than a few
          in the FAT-12 image.  FAT-32 root directories live in the data area
          and they can be of almost any size.  8192 bytes afford us enough
          room for 256 entries.

        - [6000, 8000) The Start program.  Currently a whooping < 4 KiB, it
          has here almost enough space to grow 83% more.  We really don't
          expect it to grow that much more, especially since it still has
          a lot of dead code.  Also, at the end of this region lives the
          stack, which is very likely to get relocated to the 32 KiB
          in the [08000-10000) range since the C++ code may want to place
          complex objects in it during initialization.  After that takes
          a little more shape, we'll see what the stack requirements look
          like and update as appropriate.

    * The remaining space of what used to be called *Conventional Memory*,
      the lower 640 KiB of the physical address space, is unused and free
      to load the binaries.  Currently, Elfo (30 KiB) is loaded at 20000
      and Memo (2 KiB) at 10000, but this can (and will) easily change to
      something much more flexible as these two programs take shape.
      Once Memo has relocated itself and the other Kernel binaries,
      this memory will be reused for basic memory, IO and interrupt
      management data structures.  I do not expect that they will
      ever be used for program or data pages.


The Second Stage
----------------

.. index:: Start

Also called Start, this piece of code does not have the 512-byte
restriction the bootsector has.  Currently at a whopping < 4.5 KB,
it enables the A20 line, enters the 32-bit Protected Mode and
the 64-bit Long Mode, all the while informing the user via visual
clues in the screen (assuming a screen is available), and tints
the screen in a terminalish green background with green foreground
color scheme.

It also invokes Elfo and Memo to have them bootstrap themselves
and provides pointers to functions that can be used to access
the screen.

Current behaviour:

    Elfo will initialize itself in-place, and use only static memory
    assigned to it by design in the memory map.

    Entry point signature::

        void Entry(uint8_t const* buffer, size_t size, Entry_Point* entry_point);


    Memo will initialize itself in-place, and use only static memory
    assigned to it by design in the memory map.

    Entry point signature::

        void Entry(Bios_Memory_Map_Entry const* Map, uint64_t Count);


Desired (next) behaviour:

    Start executes Elfo invoking the ``EXECUTE`` function with the
    ``BOOTSTRAP`` option.  Elfo then does a series of Memo executions
    of function ``BOOTSTRAP`` that will initialize it from the BIOS
    memory map, and then relocate both itself and Elfo's sections.
    Memo then executes a very small, contained code that will prepare a GDT
    entry and an LDT for each one of the two moduless (using facilities
    exported by ``Start``) to ensure the virtual addresses match
    the expectations from the binaries.  Then Memo initializes itself
    properly, which may involve reworking the LDT, and creating and relocating
    pages as necessary, and return to Elfo with appropriate success or
    failure indication.

    Once this is all completed, Start will bootstrap Anjo to install
    the system call and IPC mechanisms.  The former based on the ``SYSCALL``
    instruction, and the latter including message queues, arenas, signals,
    and more.  This invocation is more traditional in the sense that
    Elfo's ``EXECUTE`` function is invoked with the ``KERNEL`` option,
    which in turn invokes Memo's ``RELOCATE KERNEL`` and calls Anjo's
    entry point.  This, however, is still not regular runtime behaviour,
    because it's not initiated through a system call.

    All of the kernel modules now have the same entry point interface::

        uint64_t Entry(
            uint64_t function, uint64_t options,
            uint64_t arg_1, uint64_t arg_2,
            uint64_t arg_3, uint64_t arg_4);

    Elfo's ``EXECUTE BOOTSTRAP`` invocation looks like this::

        Entry(
            EXECUTE, BOOTSTRAP,
            elfo_buffer_address, memo_buffer_address,
            bios_memory_map_address, bios_memory_map_entry_count);

    Memo's ``BOOTSTRAP`` invocations look like this::

        Entry(
            BOOTSTRAP, INITIALIZE,
            bios_memory_map, map_element_count,
            RESERVED_MBZ, RESERVED_MBZ);

        Entry(
            BOOTSTRAP, ALLOCATE,
            new_virtual_address, buffer_size,
            process_id, RESERVED_MBZ);

        Entry(
            BOOTSTRAP, RELOCATE,
            original_address, buffer_size,
            new_virtual_address, process_id);

    The options parameter is a 64-bit argument that, for Memo and Elfo's
    ``BOOTSTRAP`` functions, has the following structure::

        +--------+----------------------+----------------------------+------+
        |  Limit | Scratch_Space_Address|       RESERVED_MBZ         |Action|
        +--------+----------------------+----------------------------+------+
                56                     32                            8      0

        Scratch_Space_Address: 24-bit base address to the buffer Memo will
        be using during its relocation process.  This is memory reserved by
        Start and can be as much as 256 KiB in size.

        Limit: 8-bit value specifying the size in KiB of the scratch space.

        Action: For Memo's BOOTSTRAP, it is a value of the
                Memo_Bootstrap_Options enum.  For Elfo's BOOTSTRAP,
                it is just the Elfo_Execute_Bootstrap value.

    The return value of a Memo's BOOTSTRAP invocation is zero on success,
    non-zero on failure.

    Elfo's ``EXECUTE KERNEL`` invocation looks like this::

        Entry(
            EXECUTE, KERNEL,
            buffer_ptr, buffer_size,
            syscall_argument_ptr, RESERVED_MBZ);

    In turn, the Syscall_Argument structure looks like this::

        struct Syscall_Argument {
            uint64_t Function;
            uint64_t Options;
            uint64_t Address_1;
            uint64_t Integer_1;
            uint64_t Address_2;
            uint64_t Integer_2;
        };

    This is (will be) documented in the appropriate sections (under Kernel),
    but it's summarized here for convenience and context.

Screenshots
-----------
