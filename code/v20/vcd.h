#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstdio>
#include <cassert>

namespace vcd {

enum var_type_t {
  var_type_wire,
  var_type_reg,
  var_type_parameter,
  var_type_integer,
};

enum timescale_t {
  timescale_s,
  timescale_ms,
  timescale_us,
  timescale_ns,
  timescale_ps,
  timescale_fs,
};

struct var_t {

  var_t()
    : id   (~0u)
    , bits (0)
    , value(0xdeadbeef)
  {
  }

  uint32_t id;
  uint32_t value;
  uint16_t bits;
};

struct vcd_t {

  vcd_t()
    : fd          (nullptr)
    , id_next     (0)
    , time_val    (~0u)
    , depth_module(0)
    , seq_defs    (false)
    , seq_began   (false)
  {
  }

  ~vcd_t() {
    if (fd) {
      fclose(fd);
    }
    fd = nullptr;
  }

  bool begin(const char *fileName, uint32_t quant, timescale_t timescale) {
    assert(!fd && "fd != nullptr");
    assert(!seq_began);
    fd = fopen(fileName, "w");
    if (!fd) {
      return false;
    }
    const char *scale = to_string(timescale);
    fprintf(fd, "$timescale %u %s $end\n", quant, scale);
    seq_began = true;
    return true;
  }

  void date(const char *date) {
    assert(fd && "fd == nullptr");
    assert(seq_defs == 1 && "only valid in defs scope");
    fprintf(fd, "$date %s $end\n", date);
  }

  void end() {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    fclose(fd);
    fd = nullptr;
    seq_began = false;
  }

  void defs_begin() {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    assert(!seq_defs);
    seq_defs = true;
  }

  void defs_end() {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    assert(seq_defs);
    assert(depth_module == 0 && "module defines out of order");
    fprintf(fd, "$enddefinitions $end\n");
    seq_defs = false;
  }

  void module_begin(const char *name) {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    fprintf(fd, "$scope module %s $end\n", name);
    ++depth_module;
  }

  void module_end() {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    assert(depth_module && "invalid module end");
    --depth_module;
    fprintf(fd, "$upscope $end\n");
  }

  void def(var_t &var, const char *name, var_type_t type, uint32_t bits) {
    assert(fd && "fd == nullptr");
    assert(bits && "invalid bits");
    assert(seq_began);
    assert(seq_defs && "only valid in defs scope");
    const char *type_str = to_string(type);
    var.id   = id_next++;
    var.bits = bits;
    fprintf(fd, "$var %s %u %u %s $end\n", type_str, bits, var.id, name);
  }

  void set(var_t &var, uint32_t data) {
    assert(fd && "fd == nullptr");
    assert(var.bits && "invalid var bits");
    assert((var.id != ~0u) && "invalid var id");
    assert(seq_began);
    assert(seq_defs == 0 && "only valid out of defs scope");
    if (var.value != data) {
      var.value = data;
      uint32_t mask = 1u << (var.bits - 1);
      fputc('b', fd);
      for (uint32_t i = 0; i < var.bits; ++i) {
        fputc((data & mask) ? '1' : '0', fd);
        data <<= 1;
      }
      fprintf(fd, " %u\n", var.id);
    }
  }

  void comment(const char *str) {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    assert(!seq_defs && "only valid out of defs scope");
    fprintf(fd, "$comment %s $end\n", str);
  }

  void timestamp(uint32_t time) {
    assert(fd && "fd == nullptr");
    assert(seq_began);
    assert(!seq_defs && "only valid out of defs scope");
    if (time != time_val) {
      fprintf(fd, "#%u\n", time);
    }
    time_val = time;
  }

  void timeinc() {
    timestamp(time_val + 1);
  }

protected:

  const char *to_string(timescale_t timescale) const {
    switch (timescale) {
    case timescale_s:  return "s";
    case timescale_ms: return "ms";
    case timescale_us: return "us";
    case timescale_ns: return "ns";
    case timescale_ps: return "ps";
    case timescale_fs: return "fs";
    default:           return "unknown_timescale";
    }
  }

  const char *to_string(var_type_t type) const {
    switch (type) {
    case var_type_wire:      return "wire";
    case var_type_reg:       return "reg";
    case var_type_parameter: return "parameter";
    case var_type_integer:   return "integer";
    default:                 return "unknown_type";
    }
  }

  FILE     *fd;
  uint32_t  id_next;
  uint32_t  time_val;
  uint32_t  depth_module;
  bool      seq_defs;
  bool      seq_began;
};

}  // namespace vcd
