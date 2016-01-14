/* Copyright (c) 2013-2016 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "isa-lr35902.h"

#include "lr35902/emitter-lr35902.h"
#include "lr35902/lr35902.h"

#define DEFINE_INSTRUCTION_LR35902(NAME, BODY) \
	static void _LR35902Instruction ## NAME (struct LR35902Core* cpu) { \
		UNUSED(cpu); \
		BODY; \
	}

DEFINE_INSTRUCTION_LR35902(NOP,);

DEFINE_INSTRUCTION_LR35902(JPFinish,
	if (cpu->condition) {
		cpu->pc = (cpu->bus << 8) | cpu->index;
		cpu->memory.setActiveRegion(cpu, cpu->pc);
		// TODO: Stall properly
		cpu->cycles += 4;
	})

DEFINE_INSTRUCTION_LR35902(JPDelay,
	cpu->executionState = LR35902_CORE_READ_PC;
	cpu->instruction = _LR35902InstructionJPFinish;
	cpu->index = cpu->bus;)

#define DEFINE_JP_INSTRUCTION_LR35902(CONDITION_NAME, CONDITION) \
	DEFINE_INSTRUCTION_LR35902(JP ## CONDITION_NAME, \
		cpu->executionState = LR35902_CORE_READ_PC; \
		cpu->instruction = _LR35902InstructionJPDelay; \
		cpu->condition = CONDITION;)

DEFINE_JP_INSTRUCTION_LR35902(, true);
DEFINE_JP_INSTRUCTION_LR35902(C, cpu->f.c);
DEFINE_JP_INSTRUCTION_LR35902(Z, cpu->f.z);
DEFINE_JP_INSTRUCTION_LR35902(NC, !cpu->f.c);
DEFINE_JP_INSTRUCTION_LR35902(NZ, !cpu->f.z);

DEFINE_INSTRUCTION_LR35902(JRFinish,
	if (cpu->condition) {
		cpu->pc += (int8_t) cpu->bus;
		cpu->memory.setActiveRegion(cpu, cpu->pc);
		// TODO: Stall properly
		cpu->cycles += 4;
	})

#define DEFINE_JR_INSTRUCTION_LR35902(CONDITION_NAME, CONDITION) \
	DEFINE_INSTRUCTION_LR35902(JR ## CONDITION_NAME, \
		cpu->executionState = LR35902_CORE_READ_PC; \
		cpu->instruction = _LR35902InstructionJRFinish; \
		cpu->condition = CONDITION;)

DEFINE_JR_INSTRUCTION_LR35902(, true);
DEFINE_JR_INSTRUCTION_LR35902(C, cpu->f.c);
DEFINE_JR_INSTRUCTION_LR35902(Z, cpu->f.z);
DEFINE_JR_INSTRUCTION_LR35902(NC, !cpu->f.c);
DEFINE_JR_INSTRUCTION_LR35902(NZ, !cpu->f.z);

#define DEFINE_AND_INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(AND ## NAME, \
		cpu->a &= OPERAND; \
		cpu->f.z = !cpu->a; \
		cpu->f.n = 0; \
		cpu->f.c = 0; \
		cpu->f.h = 1;)

#define DEFINE_XOR_INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(XOR ## NAME, \
		cpu->a ^= OPERAND; \
		cpu->f.z = !cpu->a; \
		cpu->f.n = 0; \
		cpu->f.c = 0; \
		cpu->f.h = 0;)

#define DEFINE_OR_INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(OR ## NAME, \
		cpu->a |= OPERAND; \
		cpu->f.z = !cpu->a; \
		cpu->f.n = 0; \
		cpu->f.c = 0; \
		cpu->f.h = 0;)

#define DEFINE_CP_INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(CP ## NAME, \
		int diff = cpu->a - OPERAND; \
		cpu->f.n = 1; \
		cpu->f.z = !diff; \
		cpu->f.c = diff < 0; \
		/* TODO: Find explanation of H flag */)

#define DEFINE_LDB__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDB_ ## NAME, \
		cpu->b = OPERAND;)

#define DEFINE_LDC__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDC_ ## NAME, \
		cpu->c = OPERAND;)

#define DEFINE_LDD__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDD_ ## NAME, \
		cpu->d = OPERAND;)

#define DEFINE_LDE__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDE_ ## NAME, \
		cpu->e = OPERAND;)

#define DEFINE_LDH__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDH_ ## NAME, \
		cpu->h = OPERAND;)

#define DEFINE_LDL__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDL_ ## NAME, \
		cpu->l = OPERAND;)

#define DEFINE_LDHL__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDHL_ ## NAME, \
		cpu->bus = OPERAND; \
		cpu->executionState = LR35902_CORE_MEMORY_MOVE_INDEX_STORE; \
		cpu->instruction = _LR35902InstructionLDHL_Bus;)

#define DEFINE_LDA__INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LDA_ ## NAME, \
		cpu->a = OPERAND;)

#define DEFINE_LD_INSTRUCTION_LR35902(NAME, OPERAND) \
	DEFINE_INSTRUCTION_LR35902(LD ## NAME, \
		cpu->executionState = LR35902_CORE_READ_PC; \
		cpu->instruction = _LR35902InstructionLD ## NAME ## _Bus;)

#define DEFINE_ALU_INSTRUCTION_LR35902_NOHL(NAME) \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(A, cpu->a); \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(B, cpu->b); \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(C, cpu->c); \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(D, cpu->d); \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(E, cpu->e); \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(H, cpu->h); \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(L, cpu->l);

DEFINE_INSTRUCTION_LR35902(LDHL_Bus, \
	cpu->index = LR35902ReadHL(cpu); \
	cpu->executionState = LR35902_CORE_MEMORY_MOVE_INDEX_STORE; \
	cpu->instruction = _LR35902InstructionNOP;)

DEFINE_INSTRUCTION_LR35902(LDHL, \
	cpu->executionState = LR35902_CORE_READ_PC; \
	cpu->instruction = _LR35902InstructionLDHL_Bus;)

#define DEFINE_ALU_INSTRUCTION_LR35902(NAME) \
	DEFINE_ ## NAME ## _INSTRUCTION_LR35902(Bus, cpu->bus); \
	DEFINE_INSTRUCTION_LR35902(NAME ## HL, \
		cpu->executionState = LR35902_CORE_MEMORY_MOVE_INDEX_LOAD; \
		cpu->index = LR35902ReadHL(cpu); \
		cpu->instruction = _LR35902Instruction ## NAME ## Bus;) \
	DEFINE_INSTRUCTION_LR35902(NAME, \
		cpu->executionState = LR35902_CORE_READ_PC; \
		cpu->instruction = _LR35902Instruction ## NAME ## Bus;) \
	DEFINE_ALU_INSTRUCTION_LR35902_NOHL(NAME)

DEFINE_ALU_INSTRUCTION_LR35902(AND);
DEFINE_ALU_INSTRUCTION_LR35902(XOR);
DEFINE_ALU_INSTRUCTION_LR35902(OR);
DEFINE_ALU_INSTRUCTION_LR35902(CP);

static void _LR35902InstructionLDB_Bus(struct LR35902Core*);
static void _LR35902InstructionLDC_Bus(struct LR35902Core*);
static void _LR35902InstructionLDD_Bus(struct LR35902Core*);
static void _LR35902InstructionLDE_Bus(struct LR35902Core*);
static void _LR35902InstructionLDH_Bus(struct LR35902Core*);
static void _LR35902InstructionLDL_Bus(struct LR35902Core*);
static void _LR35902InstructionLDHL_Bus(struct LR35902Core*);
static void _LR35902InstructionLDA_Bus(struct LR35902Core*);

DEFINE_ALU_INSTRUCTION_LR35902(LDB_);
DEFINE_ALU_INSTRUCTION_LR35902(LDC_);
DEFINE_ALU_INSTRUCTION_LR35902(LDD_);
DEFINE_ALU_INSTRUCTION_LR35902(LDE_);
DEFINE_ALU_INSTRUCTION_LR35902(LDH_);
DEFINE_ALU_INSTRUCTION_LR35902(LDL_);
DEFINE_ALU_INSTRUCTION_LR35902_NOHL(LDHL_);
DEFINE_ALU_INSTRUCTION_LR35902(LDA_);

DEFINE_INSTRUCTION_LR35902(LDIAFinish, \
	cpu->index |= cpu->bus << 8;
	cpu->bus = cpu->a; \
	cpu->executionState = LR35902_CORE_MEMORY_MOVE_INDEX_STORE; \
	cpu->instruction = _LR35902InstructionNOP;)

DEFINE_INSTRUCTION_LR35902(LDIADelay, \
	cpu->index = cpu->bus;
	cpu->executionState = LR35902_CORE_READ_PC; \
	cpu->instruction = _LR35902InstructionLDIAFinish;)

DEFINE_INSTRUCTION_LR35902(LDIA, \
	cpu->executionState = LR35902_CORE_READ_PC; \
	cpu->instruction = _LR35902InstructionLDIADelay;)

DEFINE_INSTRUCTION_LR35902(LDAIFinish, \
	cpu->index |= cpu->bus << 8;
	cpu->executionState = LR35902_CORE_MEMORY_MOVE_INDEX_LOAD; \
	cpu->instruction = _LR35902InstructionLDA_Bus;)

DEFINE_INSTRUCTION_LR35902(LDAIDelay, \
	cpu->index = cpu->bus;
	cpu->executionState = LR35902_CORE_READ_PC; \
	cpu->instruction = _LR35902InstructionLDAIFinish;)

DEFINE_INSTRUCTION_LR35902(LDAI, \
	cpu->executionState = LR35902_CORE_READ_PC; \
	cpu->instruction = _LR35902InstructionLDAIDelay;)

DEFINE_INSTRUCTION_LR35902(DI, cpu->irqh.setInterrupts(cpu, false));
DEFINE_INSTRUCTION_LR35902(EI, cpu->irqh.setInterrupts(cpu, true));

DEFINE_INSTRUCTION_LR35902(STUB, cpu->irqh.hitStub(cpu));

const LR35902Instruction _lr35902InstructionTable[0x100] = {
	DECLARE_LR35902_EMITTER_BLOCK(_LR35902Instruction)
};

const LR35902Instruction _lr35902CBInstructionTable[0x100] = {
	DECLARE_LR35902_CB_EMITTER_BLOCK(_LR35902Instruction)
};
