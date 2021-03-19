/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */
package ejsc;

import java.io.FileOutputStream;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import specfile.SpecFile;

import ejsc.Main.Info;
import ejsc.Main.Info.BaseBit;


public class OBCFileComposer extends OutputFileComposer {
    static final boolean DEBUG = false;

    static final boolean BIG_ENDIAN        = true;

    static final byte OBC_FILE_MAGIC       = (byte) 0xec;

    static final int FIELD_VALUE_TRUE      = 0x03;
    static final int FIELD_VALUE_FALSE     = 0x01;
    static final int FIELD_VALUE_NULL      = 0x00;
    static final int FIELD_VALUE_UNDEFINED = 0x02;

    static abstract class InsnBinaryFormatter {
        abstract int GetBytes();
        abstract long MakeSmallPrimitive(int opcode, int register, int immediate);
        abstract long MakeBigPrimitive(int opcode, int register, int index);
        abstract long MakeThreeOp(int opcode, int firstOperand, int secondOperand, int thirdOperand);
        abstract long MakeTwoOp(int opcode, int firstOperand, int secondOperand);
        abstract long MakeOneOp(int opcode, int firstOperand);
        abstract long MakeZeroOp(int opcode);
        abstract long MakeUncondJump(int opcode, int displacement);
        abstract long MakeCondJump(int opcode, int register, int displacement);
        abstract long MakeGetVar(int opcode, int link, int offset, int register);
        abstract long MakeSetVar(int opcode, int link, int offset, int register);
        abstract long MakeMakeClosureOp(int opcode, int register, int subscr);
        abstract long MakeCallOp(int opcode, int register, int nargs);
        abstract long MakeTryOp(int opcode);
        abstract long MakeUnknownOp(int opcode);
    }

    static class Insn32bitFormatter extends InsnBinaryFormatter {
        static final int OPCODE_BITS           = 8;
        static final int OPCODE_OFFSET         = 32 - OPCODE_BITS;
        static final long OPCODE_MASK          = ((1L << OPCODE_BITS) - 1) << OPCODE_OFFSET;
        static final int FIRST_OPERAND_BITS    = 8;
        static final int FIRST_OPERAND_OFFSET  = OPCODE_OFFSET - FIRST_OPERAND_BITS;
        static final long FIRST_OPERAND_MASK   = ((1L << FIRST_OPERAND_BITS) - 1) << FIRST_OPERAND_OFFSET;
        static final int SECOND_OPERAND_BITS   = 8;
        static final int SECOND_OPERAND_OFFSET = FIRST_OPERAND_OFFSET - SECOND_OPERAND_BITS;
        static final long SECOND_OPERAND_MASK  = ((1L << SECOND_OPERAND_BITS) - 1) << SECOND_OPERAND_OFFSET;
        static final int THIRD_OPERAND_BITS    = 8;
        static final int THIRD_OPERAND_OFFSET  = SECOND_OPERAND_OFFSET - THIRD_OPERAND_BITS;
        static final long THIRD_OPERAND_MASK   = ((1L << THIRD_OPERAND_BITS) - 1) << THIRD_OPERAND_OFFSET;
        static final int PRIMITIVE_BITS        = 16;
        static final int PRIMITIVE_OFFSET      = 0;
        static final long PRIMITIVE_MASK       = ((1L << PRIMITIVE_BITS) - 1) << PRIMITIVE_OFFSET;

        int GetBytes()
        {
            return 4;
        }
        long MakeSmallPrimitive(int opcode, int register, int immediate)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) immediate) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeBigPrimitive(int opcode, int register, int index)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) index) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeThreeOp(int opcode, int firstOperand, int secondOperand, int thirdOperand)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) firstOperand) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) secondOperand) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            insn |= (((long) thirdOperand) << THIRD_OPERAND_OFFSET) & THIRD_OPERAND_MASK;
            return insn;
        }
        long MakeTwoOp(int opcode, int firstOperand, int secondOperand)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) firstOperand) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) secondOperand) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            return insn;
        }
        long MakeOneOp(int opcode, int firstOperand)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) firstOperand) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            return insn;
        }
        long MakeZeroOp(int opcode)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            return insn;
        }
        long MakeUncondJump(int opcode, int displacement)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) displacement) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeCondJump(int opcode, int register, int displacement)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) displacement) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeGetVar(int opcode, int link, int offset, int register)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) link) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) offset) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            insn |= (((long) register) << THIRD_OPERAND_OFFSET) & THIRD_OPERAND_MASK;
            return insn;
        }
        long MakeSetVar(int opcode, int link, int offset, int register)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) link) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) offset) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            insn |= (((long) register) << THIRD_OPERAND_OFFSET) & THIRD_OPERAND_MASK;
            return insn;
        }
        long MakeMakeClosureOp(int opcode, int register, int subscr)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) subscr) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            return insn;
        }
        long MakeCallOp(int opcode, int register, int nargs)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) nargs) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            return insn;
        }
        long MakeTryOp(int opcode)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            return insn;
        }
        long MakeUnknownOp(int opcode)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            return insn;
        }
    }

    static class Insn64bitFormatter extends InsnBinaryFormatter {
        static final int OPCODE_BITS           = 16;
        static final int OPCODE_OFFSET         = 64 - OPCODE_BITS;
        static final long OPCODE_MASK          = ((1L << OPCODE_BITS) - 1) << OPCODE_OFFSET;
        static final int FIRST_OPERAND_BITS    = 16;
        static final int FIRST_OPERAND_OFFSET  = OPCODE_OFFSET - FIRST_OPERAND_BITS;
        static final long FIRST_OPERAND_MASK   = ((1L << FIRST_OPERAND_BITS) - 1) << FIRST_OPERAND_OFFSET;
        static final int SECOND_OPERAND_BITS   = 16;
        static final int SECOND_OPERAND_OFFSET = FIRST_OPERAND_OFFSET - SECOND_OPERAND_BITS;
        static final long SECOND_OPERAND_MASK  = ((1L << SECOND_OPERAND_BITS) - 1) << SECOND_OPERAND_OFFSET;
        static final int THIRD_OPERAND_BITS    = 16;
        static final int THIRD_OPERAND_OFFSET  = SECOND_OPERAND_OFFSET - THIRD_OPERAND_BITS;
        static final long THIRD_OPERAND_MASK   = ((1L << THIRD_OPERAND_BITS) - 1) << THIRD_OPERAND_OFFSET;
        static final int PRIMITIVE_BITS        = 32;
        static final int PRIMITIVE_OFFSET      = 0;
        static final long PRIMITIVE_MASK       = ((1L << PRIMITIVE_BITS) - 1) << PRIMITIVE_OFFSET;

        int GetBytes()
        {
            return 8;
        }
        long MakeSmallPrimitive(int opcode, int register, int immediate)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) immediate) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeBigPrimitive(int opcode, int register, int index)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) index) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeThreeOp(int opcode, int firstOperand, int secondOperand, int thirdOperand)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) firstOperand) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) secondOperand) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            insn |= (((long) thirdOperand) << THIRD_OPERAND_OFFSET) & THIRD_OPERAND_MASK;
            return insn;
        }
        long MakeTwoOp(int opcode, int firstOperand, int secondOperand)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) firstOperand) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) secondOperand) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            return insn;
        }
        long MakeOneOp(int opcode, int firstOperand)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) firstOperand) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            return insn;
        }
        long MakeZeroOp(int opcode)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            return insn;
        }
        long MakeUncondJump(int opcode, int displacement)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) displacement) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeCondJump(int opcode, int register, int displacement)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) displacement) << PRIMITIVE_OFFSET) & PRIMITIVE_MASK;
            return insn;
        }
        long MakeGetVar(int opcode, int link, int offset, int register)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) link) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) offset) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            insn |= (((long) register) << THIRD_OPERAND_OFFSET) & THIRD_OPERAND_MASK;
            return insn;
        }
        long MakeSetVar(int opcode, int link, int offset, int register)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) link) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) offset) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            insn |= (((long) register) << THIRD_OPERAND_OFFSET) & THIRD_OPERAND_MASK;
            return insn;
        }
        long MakeMakeClosureOp(int opcode, int register, int subscr)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) subscr) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            return insn;
        }
        long MakeCallOp(int opcode, int register, int nargs)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            insn |= (((long) register) << FIRST_OPERAND_OFFSET) & FIRST_OPERAND_MASK;
            insn |= (((long) nargs) << SECOND_OPERAND_OFFSET) & SECOND_OPERAND_MASK;
            return insn;
        }
        long MakeTryOp(int opcode)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            return insn;
        }
        long MakeUnknownOp(int opcode)
        {
            long insn = ((long) opcode) << OPCODE_OFFSET;
            return insn;
        }
    }

    static class OBCInstruction {
        static final int CHAR_BITS             = 8;

        enum Format {
            SMALLPRIMITIVE,
            BIGPRIMITIVE,
            THREEOP,
            TWOOP,
            ONEOP,
            ZEROOP,
            UNCONDJUMP,
            CONDJUMP,
            GETVAR,
            SETVAR,
            MAKECLOSUREOP,
            CALLOP,
            TRYOP,
            UNKNOWNOP
        }

        static OBCInstruction createSmallPrimitive(String insnName, int opcode, int register, int immediate) {
            return new OBCInstruction(insnName, opcode, Format.SMALLPRIMITIVE, register, immediate, 0);
        }

        static OBCInstruction createBigPrimitive(String insnName, int opcode, int register, int index) {
            return new OBCInstruction(insnName, opcode, Format.BIGPRIMITIVE, register, index, 0);
        }

        static OBCInstruction createThreeOp(String insnName, int opcode, int firstOperand, int secondOperand, int thirdOperand) {
            return new OBCInstruction(insnName, opcode, Format.THREEOP, firstOperand, secondOperand, thirdOperand);
        }

        static OBCInstruction createTwoOp(String insnName, int opcode, int firstOperand, int secondOperand) {
            return new OBCInstruction(insnName, opcode, Format.TWOOP, firstOperand, secondOperand, 0);
        }

        static OBCInstruction createOneOp(String insnName, int opcode, int firstOperand) {
            return new OBCInstruction(insnName, opcode, Format.ONEOP, firstOperand, 0, 0);
        }

        static OBCInstruction createZeroOp(String insnName, int opcode) {
            return new OBCInstruction(insnName, opcode, Format.ZEROOP, 0, 0, 0);
        }

        static OBCInstruction createUncondJump(String insnName, int opcode, int displacement) {
            return new OBCInstruction(insnName, opcode, Format.UNCONDJUMP, 0, displacement, 0);
        }

        static OBCInstruction createCondJump(String insnName, int opcode, int register, int displacement) {
            return new OBCInstruction(insnName, opcode, Format.CONDJUMP, register, displacement, 0);
        }

        static OBCInstruction createGetVar(String insnName, int opcode, int link, int offset, int register) {
            return new OBCInstruction(insnName, opcode, Format.GETVAR, link, offset, register);
        }

        static OBCInstruction createSetVar(String insnName, int opcode, int link, int offset, int register) {
            return new OBCInstruction(insnName, opcode, Format.SETVAR, link, offset, register);
        }

        static OBCInstruction createMakeClosureOp(String insnName, int opcode, int register, int subscr) {
            return new OBCInstruction(insnName, opcode, Format.MAKECLOSUREOP, register, subscr, 0);
        }

        static OBCInstruction createCallOp(String insnName, int opcode, int register, int nargs) {
            return new OBCInstruction(insnName, opcode, Format.MAKECLOSUREOP, register, nargs, 0);
        }

        String insnName;  /* debug */
        int opcode;
        Format format;
        int firstOperand, secondOperand, thirdOperand;

        OBCInstruction(String insnName, int opcode, Format format, int a, int b, int c) {
            this.insnName = insnName;
            this.opcode = opcode;
            this.format = format;
            this.firstOperand = a;
            this.secondOperand = b;
            this.thirdOperand = c;
        }

        /**
         * Returns binary representation of the instruction.
         * @return binary representation of this instruction.
         */
        byte[] getBytes(InsnBinaryFormatter formatter) {
            long insn;
            int instsBytes = formatter.GetBytes();
            switch (format) {
            case SMALLPRIMITIVE:
                insn = formatter.MakeBigPrimitive(opcode, firstOperand, secondOperand);
                break;
            case BIGPRIMITIVE:
                insn = formatter.MakeBigPrimitive(opcode, firstOperand, secondOperand);
                break;
            case THREEOP:
                insn = formatter.MakeThreeOp(opcode, firstOperand, secondOperand, thirdOperand);
                break;
            case TWOOP:
                insn = formatter.MakeTwoOp(opcode, firstOperand, secondOperand);
                break;
            case ONEOP:
                insn = formatter.MakeOneOp(opcode, firstOperand);
                break;
            case ZEROOP:
                insn = formatter.MakeZeroOp(opcode);
                break;
            case UNCONDJUMP:
                insn = formatter.MakeUncondJump(opcode, secondOperand);
                break;
            case CONDJUMP:
                insn = formatter.MakeCondJump(opcode, firstOperand, secondOperand);
                break;
            case GETVAR:
                insn = formatter.MakeGetVar(opcode, firstOperand, secondOperand, thirdOperand);
                break;
            case SETVAR:
                insn = formatter.MakeSetVar(opcode, firstOperand, secondOperand, thirdOperand);
                break;
            case MAKECLOSUREOP:
                insn = formatter.MakeMakeClosureOp(opcode, firstOperand, secondOperand);
                break;
            case CALLOP:
                insn = formatter.MakeCallOp(opcode, firstOperand, secondOperand);
                break;
            case TRYOP:
                insn = formatter.MakeTryOp(opcode);
                break;
            case UNKNOWNOP:
                insn = formatter.MakeUnknownOp(opcode);
                break;
            default:
                throw new Error("Unknown instruction format");    
            }

            if (DEBUG)
                System.out.println(String.format("insn: %016x  %s", insn, insnName));

            if (BIG_ENDIAN)
                insn = Long.reverseBytes(insn);

            insn = insn >> (CHAR_BITS * (8 - instsBytes));
            byte[] bytes = new byte[instsBytes];
            for (int i = 0; i < instsBytes; i++)
                bytes[i] = (byte) (insn >> (CHAR_BITS * i));

            return bytes;
        }
    }

    class OBCFunction implements CodeBuffer {
        int functionNumberOffset;

        /* function header */
        int callEntry;
        int sendEntry;
        int numberOfLocals;

        ConstantTable constants;
        List<OBCInstruction> instructions;

        OBCFunction(BCBuilder.FunctionBCBuilder fb, int functionNumberOffset) {
            this.functionNumberOffset = functionNumberOffset;

            List<BCode> bcodes = fb.getInstructions();
            this.callEntry = fb.callEntry.dist(0);
            this.sendEntry = fb.sendEntry.dist(0);
            this.numberOfLocals = fb.numberOfLocals;

            constants = new ConstantTable();
            instructions = new ArrayList<OBCInstruction>(bcodes.size());
            for (BCode bc: bcodes)
                bc.emit(this);
        }

        int getOpcode(String insnName, SrcOperand... srcs) {
            String decorated = OBCFileComposer.decorateInsnName(insnName, srcs);
            if (decorated == null)
                return spec.getOpcodeIndex(insnName);
            else
                return spec.getOpcodeIndex(decorated);
        }

        int fieldBitsOf(SrcOperand src) {
            if (src instanceof RegisterOperand) {
                Register r = ((RegisterOperand) src).get();
                int n = r.getRegisterNumber();
                return n;
            } else if (src instanceof FixnumOperand) {
                int n = ((FixnumOperand) src).get();
                return n;
            } else if (src instanceof FlonumOperand) {
                double n = ((FlonumOperand) src).get();
                int index = constants.lookup(n);
                return index;
            } else if (src instanceof StringOperand) {
                String s = ((StringOperand) src).get();
                int index = constants.lookup(s);
                return index;
            } else if (src instanceof SpecialOperand) {
                SpecialOperand.V v = ((SpecialOperand) src).get();
                switch (v) {
                case TRUE:
                    return (FIELD_VALUE_TRUE << Info.PTAGSize) | Info.SpecPTAGValue;
                case FALSE:
                    return (FIELD_VALUE_FALSE << Info.PTAGSize) | Info.SpecPTAGValue;
                case NULL:
                    return (FIELD_VALUE_NULL << Info.PTAGSize) | Info.SpecPTAGValue;
                case UNDEFINED:
                    return (FIELD_VALUE_UNDEFINED << Info.PTAGSize) | Info.SpecPTAGValue;
                default:
                    throw new Error("Unknown special");
                }
            } else
                throw new Error("Unknown source operand");
        }

        @Override
        public void addFixnumSmallPrimitive(String insnName, boolean log, Register dst, int n) {
            if (isFixnumRange(n)) {
                int opcode = getOpcode(insnName);
                int a = dst.getRegisterNumber();
                int b = n;
                OBCInstruction insn = OBCInstruction.createSmallPrimitive(insnName, opcode, a, b);
                instructions.add(insn);
            }
            else {
                throw new Error("Fixnum; Out of range");
            }
        }
        @Override
        public void addNumberBigPrimitive(String insnName, boolean log, Register dst, double n) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            int b = constants.lookup(n);
            OBCInstruction insn = OBCInstruction.createBigPrimitive(insnName, opcode, a, b);
            instructions.add(insn);

        }
        @Override
        public void addStringBigPrimitive(String insnName, boolean log, Register dst, String s) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            int b = constants.lookup(s);
            OBCInstruction insn = OBCInstruction.createBigPrimitive(insnName, opcode, a, b);
            instructions.add(insn);
        }
        @Override
        public void addSpecialSmallPrimitive(String insnName, boolean log, Register dst, SpecialValue v) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            int b;
            switch (v) {
            case TRUE:
                b =  (FIELD_VALUE_TRUE << Info.PTAGSize) | Info.SpecPTAGValue; break;
            case FALSE:
                b =  (FIELD_VALUE_FALSE << Info.PTAGSize) | Info.SpecPTAGValue; break;
            case NULL:
                b =  (FIELD_VALUE_NULL << Info.PTAGSize) | Info.SpecPTAGValue; break;
            case UNDEFINED:
                b =  (FIELD_VALUE_UNDEFINED << Info.PTAGSize) | Info.SpecPTAGValue; break;
            default:
                throw new Error("Unknown special");
            }
            OBCInstruction insn = OBCInstruction.createSmallPrimitive(insnName, opcode, a, b);
            instructions.add(insn);
        }
        @Override
        public void addRegexp(String insnName, boolean log, Register dst, int flag, String ptn) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            int c = constants.lookup(ptn);
            OBCInstruction insn = OBCInstruction.createThreeOp(insnName, opcode, a, flag, c);
            instructions.add(insn);
        }
        @Override
        public void addRXXThreeOp(String insnName, boolean log, Register dst, SrcOperand src1, SrcOperand src2) {
            int opcode = getOpcode(insnName, src1, src2);
            int a = dst.getRegisterNumber();
            int b = fieldBitsOf(src1);
            int c = fieldBitsOf(src2);
            OBCInstruction insn = OBCInstruction.createThreeOp(insnName, opcode, a, b, c);
            instructions.add(insn);
        }
        @Override
        public void addXXXThreeOp(String insnName, boolean log, SrcOperand src1, SrcOperand src2, SrcOperand src3) {
            int opcode = getOpcode(insnName, src1, src2, src3);
            int a = fieldBitsOf(src1);
            int b = fieldBitsOf(src2);
            int c = fieldBitsOf(src3);
            OBCInstruction insn = OBCInstruction.createThreeOp(insnName, opcode, a, b, c);
            instructions.add(insn);
        }
        @Override
        public void addXIXThreeOp(String insnName, boolean log, SrcOperand src1, int index, SrcOperand src2) {
            int opcode = getOpcode(insnName, src1, src2);
            int a = fieldBitsOf(src1);
            int c = fieldBitsOf(src2);
            OBCInstruction insn = OBCInstruction.createThreeOp(insnName, opcode, a, index, c);
            instructions.add(insn);
        }
        @Override
        public void addRXTwoOp(String insnName, boolean log, Register dst, SrcOperand src) {
            int opcode = getOpcode(insnName, src);
            int a = dst.getRegisterNumber();
            int b = fieldBitsOf(src);
            OBCInstruction insn = OBCInstruction.createTwoOp(insnName, opcode, a, b);
            instructions.add(insn);
        }
        @Override
        public void addXXTwoOp(String insnName, boolean log, SrcOperand src1, SrcOperand src2) {
            int opcode = getOpcode(insnName, src1, src2);
            int a = fieldBitsOf(src1);
            int b = fieldBitsOf(src2);
            OBCInstruction insn = OBCInstruction.createTwoOp(insnName, opcode, a, b);
            instructions.add(insn);
        }
        @Override
        public void addROneOp(String insnName, boolean log, Register dst) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            OBCInstruction insn = OBCInstruction.createOneOp(insnName, opcode, a);
            instructions.add(insn);
        }
        @Override
        public void addXOneOp(String insnName, boolean log, SrcOperand src) {
            int opcode = getOpcode(insnName, src);
            int a = fieldBitsOf(src);
            OBCInstruction insn = OBCInstruction.createOneOp(insnName, opcode, a);
            instructions.add(insn);
        }
        @Override
        public void addIOneOp(String insnName, boolean log, int n) {
            int opcode = getOpcode(insnName);
            OBCInstruction insn = OBCInstruction.createOneOp(insnName, opcode, n);
            instructions.add(insn);
        }
        @Override
        public void addZeroOp(String insnName, boolean log) {
            int opcode = getOpcode(insnName);
            OBCInstruction insn = OBCInstruction.createZeroOp(insnName, opcode);
            instructions.add(insn);
        }
        @Override
        public void addNewFrameOp(String insnName, boolean log, int len, boolean mkargs) {
            int opcode = getOpcode(insnName);
            int b = mkargs ? 1 : 0;
            OBCInstruction insn = OBCInstruction.createTwoOp(insnName, opcode, len, b);
            instructions.add(insn);
        }
        @Override
        public void addGetVar(String insnName, boolean log, Register dst, int link, int index) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            OBCInstruction insn = OBCInstruction.createGetVar(insnName, opcode, a, link, index);
            instructions.add(insn);
        }
        @Override
        public void addSetVar(String insnName, boolean log, int link, int index, SrcOperand src) {
            int opcode = getOpcode(insnName, src);
            int c = fieldBitsOf(src);
            OBCInstruction insn = OBCInstruction.createSetVar(insnName, opcode, link, index, c);
            instructions.add(insn);
        }
        @Override
        public void addMakeClosureOp(String insnName, boolean log, Register dst, int index) {
            int opcode = getOpcode(insnName);
            int a = dst.getRegisterNumber();
            // int b = index + functionNumberOffset;
            int b = index;
            OBCInstruction insn = OBCInstruction.createMakeClosureOp(insnName, opcode, a, b);
            instructions.add(insn);
        }
        @Override
        public void addXICall(String insnName, boolean log, SrcOperand fun, int nargs) {
            int opcode = getOpcode(insnName, fun);
            int a = fieldBitsOf(fun);
            OBCInstruction insn = OBCInstruction.createCallOp(insnName, opcode, a, nargs);
            instructions.add(insn);                    
        }
        @Override
        public void addRXCall(String insnName, boolean log, Register dst, SrcOperand fun) {
            int opcode = getOpcode(insnName, fun);
            int a = dst.getRegisterNumber();
            int b = fieldBitsOf(fun);
            OBCInstruction insn = OBCInstruction.createCallOp(insnName, opcode, a, b);
            instructions.add(insn);
        }
        @Override
        public void addUncondJump(String insnName, boolean log, int disp) {
            int opcode = getOpcode(insnName);
            OBCInstruction insn = OBCInstruction.createUncondJump(insnName, opcode, disp);
            instructions.add(insn);
        }
        @Override
        public void addCondJump(String insnName, boolean log, SrcOperand test, int disp) {
            int opcode = getOpcode(insnName, test);
            int a = fieldBitsOf(test);
            OBCInstruction insn = OBCInstruction.createCondJump(insnName, opcode, a, disp);
            instructions.add(insn);
        }
    }

    List<OBCFunction> obcFunctions;
    BaseBit basebit;

    OBCFileComposer(BCBuilder compiledFunctions, int functionNumberOffset, SpecFile spec, BaseBit basebit) {
        super(spec);
        this.basebit = basebit;
        List<BCBuilder.FunctionBCBuilder> fbs = compiledFunctions.getFunctionBCBuilders();
        obcFunctions = new ArrayList<OBCFunction>(fbs.size());
        for (BCBuilder.FunctionBCBuilder fb: fbs) {
            OBCFunction out = new OBCFunction(fb, functionNumberOffset);
            obcFunctions.add(out);
        }
    }

    private void outputByte(OutputStream out, byte v) throws IOException {
        if (DEBUG)
            System.out.println(String.format("byte: %02x", v));
        out.write(v);
    }

    private void outputShort(OutputStream out, int v) throws IOException {
        if (DEBUG)
            System.out.println(String.format("short: %04x", v));
        if (BIG_ENDIAN)
            v = Integer.reverseBytes(v << 16);
        out.write((byte)(v & 0xff));
        out.write((byte)((v >> 8) & 0xff));
    }

    private void outputLong(OutputStream out, long v) throws IOException {
        if (DEBUG)
            System.out.println(String.format("short: %016x", v));
        if (BIG_ENDIAN)
            v = Long.reverseBytes(v);
        for (int i = 0; i < 8; i++)
            out.write((byte) ((v >> (8 * i)) & 0xff));
    }

    boolean isFixnumRange(int n) {
        switch(this.basebit) {
        case BIT_32:
            return -0x80000000 <= n && n < 0x7FFFFFFF;
        case BIT_64:
            return true;
        default:
            return false;
        }
    }

    /**
     * Output instruction to the file.
     * @param fileName file name to be output to.
     */
    void output(String fileName) {
        try {
            FileOutputStream out = new FileOutputStream(fileName);

            outputByte(out, OBC_FILE_MAGIC);
            outputByte(out, spec.getFingerprint());
            InsnBinaryFormatter formatter;
            switch(this.basebit) {
            case BIT_32:
                formatter = new Insn32bitFormatter();
                break;
            case BIT_64:
                formatter = new Insn64bitFormatter();
                break;
            default:
                throw(new Error("Unknown basebit option"));
            }

            /* File header */
            outputShort(out, obcFunctions.size());

            /* Function */
            for (OBCFunction fun: obcFunctions) {
                /* Function header */
                outputShort(out, fun.callEntry);
                outputShort(out, fun.sendEntry);
                outputShort(out, fun.numberOfLocals);
                outputShort(out, fun.instructions.size());
                outputShort(out, fun.constants.size());

                /* Instructions */
                for (OBCInstruction insn: fun.instructions)
                    out.write(insn.getBytes(formatter));               

                /* Constant pool */
                for (Object v: fun.constants.getConstants()) {
                    if (v instanceof Double) {
                        long bits = Double.doubleToLongBits((Double) v);
                        outputShort(out, 8);  // size
                        outputLong(out, bits);
                    } else if (v instanceof String) {
                        String s = (String) v;
                        outputShort(out, s.length() + 1); // size
                        if (DEBUG)
                            System.out.println("string: "+s);
                        out.write(s.getBytes());
                        out.write('\0');
                    } else
                        throw new Error("Unknown constant");
                }
            }
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }
}
