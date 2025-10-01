# XV6-RISCV Port for OrangePi RV2

This repository contains an in-progress port of MIT's xv6 teaching operating system from QEMU to bare-metal OrangePi RV2 hardware.

## Hardware Target

**Board**: OrangePi RV2  
**SoC**: Ky(R) X1 - 8-core RISC-V processor  
**Architecture**: rv64imafdcv with extensive extensions  
**RAM**: 8 GB  
**Goal**: Run xv6 directly on hardware (bare metal, no hypervisor)

## Repository Status

🔴 **Pre-Alpha** - Currently gathering hardware specifications

### What's Done
- ✅ Repository cloned and set up
- ✅ Hardware information gathered from running system
- ✅ Porting plan created
- ✅ Key differences from QEMU documented

### What's Next
- ⏳ Extract specifications from hardware manuals
- ⏳ Create hardware configuration header
- ⏳ Modify kernel for OrangePi addresses
- ⏳ Build and test minimal boot

## Documentation

Start here to understand the port:

1. **[HARDWARE_SUMMARY.md](HARDWARE_SUMMARY.md)** - What we know about the OrangePi RV2
2. **[PORTING_PLAN.md](PORTING_PLAN.md)** - Detailed porting strategy (6 phases)
3. **[PDF_EXTRACTION_GUIDE.md](PDF_EXTRACTION_GUIDE.md)** - What to extract from manuals

## Key Hardware Differences from QEMU

| Feature | QEMU virt | OrangePi RV2 |
|---------|-----------|--------------|
| UART | 0x10000000, IRQ 10 | 0xd4017000, IRQ 74 |
| PLIC | 0x0c000000 | 0xe0000000 |
| Timer | CLINT @ 0x02000000 | sstc extension (no CLINT) |
| Cores | 1-8 configurable | 8 fixed |
| RAM | 128 MB | 8 GB |
| Disk | VirtIO | SD/MMC (needs driver) |

## Building (Not Yet Functional)

```bash
# Original QEMU build (still works):
make qemu

# OrangePi build (TODO):
make kernel-orangepi    # Build for OrangePi
make deploy             # Copy to SD card
```

## Project Structure

```
xv6_riscv_port_orangePi-RV2/
├── README_PORT.md              ← This file
├── HARDWARE_SUMMARY.md         ← Known hardware specs
├── PORTING_PLAN.md            ← Detailed porting strategy
├── PDF_EXTRACTION_GUIDE.md    ← What to look for in docs
├── kernel/                    ← Kernel source (unmodified)
│   ├── memlayout.h           ← Needs update for OrangePi
│   ├── uart.c                ← May need modification
│   ├── plic.c                ← Needs address update
│   ├── start.c               ← Needs timer update
│   └── ...
├── user/                      ← User programs (unmodified)
└── Makefile                   ← Needs OrangePi targets
```

## Development Phases

### Phase 1: Minimal Boot (Current Goal)
- Goal: Print "Hello World" via UART
- Status: Awaiting hardware specifications from manuals

### Phase 2: Multi-Core
- Goal: Boot all 8 cores
- Status: Not started

### Phase 3: Interrupts & Timers
- Goal: PLIC and timer interrupts working
- Status: Not started

### Phase 4: Virtual Memory & Processes
- Goal: Full OS functionality
- Status: Not started

### Phase 5: Storage
- Goal: SD/MMC driver or alternative
- Status: Not started

### Phase 6: Additional Peripherals
- Goal: Network, GPIO, etc.
- Status: Not started

## Contributing

This is a learning project. Key areas where help is needed:

1. **Hardware Documentation**: Extract specs from PDFs
2. **Driver Development**: UART, SD/MMC, etc.
3. **Testing**: Boot testing and debugging
4. **Documentation**: Keep docs updated

## Resources

### Original XV6
- [MIT 6.1810 Course](https://pdos.csail.mit.edu/6.1810/)
- [XV6 Book](https://pdos.csail.mit.edu/6.828/2023/xv6/book-riscv-rev3.pdf)
- [Original Repository](https://github.com/mit-pdos/xv6-riscv)

### OrangePi RV2
- Hardware manuals (see PDF_EXTRACTION_GUIDE.md)
- [OrangePi Official Site](http://www.orangepi.org/)

### RISC-V
- [RISC-V Specifications](https://riscv.org/technical/specifications/)
- [RISC-V SBI Specification](https://github.com/riscv-non-isa/riscv-sbi-doc)

## License

XV6 is distributed under the MIT License (see original LICENSE file).
This port maintains the same license.

## Contact / Status Updates

Check git commit history for latest progress.

---

**Last Updated**: October 2025  
**Status**: Pre-Alpha - Planning Phase
