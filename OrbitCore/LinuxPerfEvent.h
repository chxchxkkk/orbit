//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------

//-----------------------------------
// Author: Florian Kuebler
//-----------------------------------

#pragma once

#include "LinuxPerfUtils.h"
#include "OrbitFunction.h"
#include "PrintVar.h"

using namespace LinuxPerfUtils;

class LinuxPerfEventVisitor;

// This base class will be used in order to do processing of
// different perf events using the visitor pattern.
// The data of the perf events will be copied from the ring buffer
// via memcpy directly into the concrete subclass (depending on the
// event type).
// The target of the memcpy will be a field "ring_buffer_data" that
// must be present for the subclass at compile time.
// As the perf_event_open ring buffer is 8-byte aligned, this field
// might also need to be extended with dummy bytes at the end of the record.

class LinuxPerfEvent {
 public:
  virtual uint64_t Timestamp() = 0;
  virtual void accept(LinuxPerfEventVisitor* a_Visitor) = 0;
};

struct __attribute__((__packed__)) perf_context_switch_event {
  perf_event_header header;
  perf_sample_id sample_id;
};

class LinuxContextSwitchEvent : public LinuxPerfEvent {
 public:
  perf_context_switch_event ring_buffer_data{};

  uint64_t Timestamp() override { return ring_buffer_data.sample_id.time; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;

  pid_t PID() { return ring_buffer_data.sample_id.pid; }
  pid_t TID() { return ring_buffer_data.sample_id.tid; }
  uint32_t CPU() { return ring_buffer_data.sample_id.cpu; }
  bool IsSwitchOut() {
    return ring_buffer_data.header.misc & PERF_RECORD_MISC_SWITCH_OUT;
  }
  bool IsSwitchIn() { return !IsSwitchOut(); }
};

struct __attribute__((__packed__)) perf_context_switch_cpu_wide_event {
  perf_event_header header;
  uint32_t next_prev_pid;
  uint32_t next_prev_tid;
  perf_sample_id sample_id;
};

class LinuxSystemWideContextSwitchEvent : public LinuxPerfEvent {
 public:
  perf_context_switch_cpu_wide_event ring_buffer_data{};

  uint64_t Timestamp() override { return ring_buffer_data.sample_id.time; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;

  pid_t PrevPID() {
    return IsSwitchOut() ? ring_buffer_data.sample_id.pid
                         : ring_buffer_data.next_prev_pid;
  }

  pid_t PrevTID() {
    return IsSwitchOut() ? ring_buffer_data.sample_id.tid
                         : ring_buffer_data.next_prev_tid;
  }

  pid_t NextPID() {
    return IsSwitchOut() ? ring_buffer_data.next_prev_pid
                         : ring_buffer_data.sample_id.pid;
  }

  pid_t NextTID() {
    return IsSwitchOut() ? ring_buffer_data.next_prev_tid
                         : ring_buffer_data.sample_id.tid;
  }

  bool IsSwitchOut() {
    return ring_buffer_data.header.misc & PERF_RECORD_MISC_SWITCH_OUT;
  }

  bool IsSwitchIn() { return !IsSwitchOut(); }

  uint32_t CPU() { return ring_buffer_data.sample_id.cpu; }
};

struct __attribute__((__packed__)) perf_fork_exit_event {
  struct perf_event_header header;
  uint32_t pid, ppid;
  uint32_t tid, ptid;
  uint64_t time;
  struct perf_sample_id sample_id;
};

class LinuxForkEvent : public LinuxPerfEvent {
 public:
  perf_fork_exit_event ring_buffer_data{};

  uint64_t Timestamp() override { return ring_buffer_data.time; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;

  pid_t PID() { return ring_buffer_data.pid; }
  pid_t ParentPID() { return ring_buffer_data.ppid; }
  pid_t TID() { return ring_buffer_data.tid; }
  pid_t ParentTID() { return ring_buffer_data.ptid; }
};

class LinuxExitEvent : public LinuxPerfEvent {
 public:
  perf_fork_exit_event ring_buffer_data{};

  uint64_t Timestamp() override { return ring_buffer_data.time; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;

  pid_t PID() { return ring_buffer_data.pid; }
  pid_t ParentPID() { return ring_buffer_data.ppid; }
  pid_t TID() { return ring_buffer_data.tid; }
  pid_t ParentTID() { return ring_buffer_data.ptid; }
};

struct __attribute__((__packed__)) perf_lost_event {
  struct perf_event_header header;
  uint64_t id;
  uint64_t lost;
  struct perf_sample_id sample_id;
};

class LinuxPerfLostEvent : public LinuxPerfEvent {
 public:
  perf_lost_event ring_buffer_data{};

  uint64_t Timestamp() override { return ring_buffer_data.sample_id.time; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;

  uint64_t Lost() { return ring_buffer_data.lost; }
};

template <typename perf_record_data_t>
class LinuxPerfEventRecord : public LinuxPerfEvent {
 public:
  perf_record_data_t ring_buffer_data;

  uint64_t Timestamp() override {
    return ring_buffer_data.basic_sample_data.time;
  }

  pid_t PID() { return ring_buffer_data.basic_sample_data.pid; }
  pid_t TID() { return ring_buffer_data.basic_sample_data.tid; }
  uint32_t CPU() { return ring_buffer_data.basic_sample_data.cpu; }
};

struct __attribute__((__packed__)) perf_empty_record {
  perf_event_header header;
  perf_sample_id basic_sample_data;
};

struct __attribute__((__packed__)) perf_record_with_stack {
  perf_event_header header;
  perf_sample_id basic_sample_data;
  perf_sample_regs_user_all register_data;
  perf_sample_stack_user stack_data;
};

namespace {
std::array<uint64_t, PERF_REG_X86_64_MAX>
perf_sample_regs_user_all_to_register_array(
    const perf_sample_regs_user_all& regs) {
  std::array<uint64_t, PERF_REG_X86_64_MAX> registers{};
  registers[PERF_REG_X86_AX] = regs.ax;
  registers[PERF_REG_X86_BX] = regs.bx;
  registers[PERF_REG_X86_CX] = regs.cx;
  registers[PERF_REG_X86_DX] = regs.dx;
  registers[PERF_REG_X86_SI] = regs.si;
  registers[PERF_REG_X86_DI] = regs.di;
  registers[PERF_REG_X86_BP] = regs.bp;
  registers[PERF_REG_X86_SP] = regs.sp;
  registers[PERF_REG_X86_IP] = regs.ip;
  registers[PERF_REG_X86_FLAGS] = regs.flags;
  registers[PERF_REG_X86_CS] = regs.cs;
  registers[PERF_REG_X86_SS] = regs.ss;
  // Registers ds, es, fs, gs do not actually exist.
  registers[PERF_REG_X86_DS] = 0ul;
  registers[PERF_REG_X86_ES] = 0ul;
  registers[PERF_REG_X86_FS] = 0ul;
  registers[PERF_REG_X86_GS] = 0ul;
  registers[PERF_REG_X86_R8] = regs.r8;
  registers[PERF_REG_X86_R9] = regs.r9;
  registers[PERF_REG_X86_R10] = regs.r10;
  registers[PERF_REG_X86_R11] = regs.r11;
  registers[PERF_REG_X86_R12] = regs.r12;
  registers[PERF_REG_X86_R13] = regs.r13;
  registers[PERF_REG_X86_R14] = regs.r14;
  registers[PERF_REG_X86_R15] = regs.r15;
  return registers;
}
}  // namespace

class LinuxStackSampleEvent
    : public LinuxPerfEventRecord<perf_record_with_stack> {
 public:
  std::array<uint64_t, PERF_REG_X86_64_MAX> Registers() {
    return perf_sample_regs_user_all_to_register_array(
        ring_buffer_data.register_data);
  }

  const char* StackDump() { return ring_buffer_data.stack_data.data; }
  uint64_t StackSize() { return ring_buffer_data.stack_data.dyn_size; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;
};

template <typename perf_record_data_t>
class AbstractLinuxUprobeEvent
    : public LinuxPerfEventRecord<perf_record_data_t> {
 public:
  Function* GetFunction() { return m_Function; }
  void SetFunction(Function* a_Function) { m_Function = a_Function; }

 private:
  Function* m_Function = nullptr;
};

class LinuxUprobeEvent : public AbstractLinuxUprobeEvent<perf_empty_record> {
 public:
  void accept(LinuxPerfEventVisitor* a_Visitor) override;
};

class LinuxUprobeEventWithStack
    : public AbstractLinuxUprobeEvent<perf_record_with_stack> {
 public:
  std::array<uint64_t, PERF_REG_X86_64_MAX> Registers() {
    return perf_sample_regs_user_all_to_register_array(
        ring_buffer_data.register_data);
  }

  const char* StackDump() { return ring_buffer_data.stack_data.data; }
  uint64_t StackSize() { return ring_buffer_data.stack_data.dyn_size; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;
};

class LinuxUretprobeEvent : public AbstractLinuxUprobeEvent<perf_empty_record> {
 public:
  void accept(LinuxPerfEventVisitor* a_Visitor) override;
};

class LinuxUretprobeEventWithStack
    : public AbstractLinuxUprobeEvent<perf_record_with_stack> {
 public:
  std::array<uint64_t, PERF_REG_X86_64_MAX> Registers() {
    return perf_sample_regs_user_all_to_register_array(
        ring_buffer_data.register_data);
  }

  const char* StackDump() { return ring_buffer_data.stack_data.data; }
  uint64_t StackSize() { return ring_buffer_data.stack_data.dyn_size; }

  void accept(LinuxPerfEventVisitor* a_Visitor) override;
};