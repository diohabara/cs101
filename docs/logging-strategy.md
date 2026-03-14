# Logging Strategy

- Logging is a first-class teaching tool.
- Each stage should emit trace entries with:
  - component
  - stage
  - event
  - key state
- Early CPU stages should log every instruction step.
- Memory stages should log effective addresses, reads, writes, and the values involved.
- Stack stages should log pushes, pops, calls, returns, and the resulting stack pointer.
- Paging stages should log virtual addresses, translation steps, and the resolved physical address.
