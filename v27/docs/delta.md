# Delta

- Phase 3（ハイパーバイザ編）の開始
- 新しい概念: VT-x, VMX, CPUID 検出, VMXON/VMXOFF, IA32_FEATURE_CONTROL MSR, CR4.VMXE
- 新しいファイル: vmx.h, vmx_detect.c
- v21 の GDT/IDT/Timer カーネルの上に VMX 検出レイヤーを追加
