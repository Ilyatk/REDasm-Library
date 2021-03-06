#ifndef AVR8_H
#define AVR8_H

// http://roncella.iet.unipi.it/Didattica/Corsi/Elettronica/Risorse/Atmel-0856-AVR-Instruction-Set-Manual.pdf
// Based on: https://github.com/vsergeev/vavrdisasm

#include "../../plugins/plugins.h"
#include "avr8_decoder.h"

namespace REDasm {

class AVR8Assembler: public AssemblerPlugin
{
    PLUGIN_NAME("AVR8")

    private:
        typedef std::function<bool(u16, const InstructionPtr& instruction)> OpCodeCallback;

    public:
        AVR8Assembler();
        virtual Printer* createPrinter(DisassemblerAPI *disassembler) const;

    private:
        void compileInstruction(const InstructionPtr& instruction, const AVR8Operand &avrop, size_t opindex);

    protected:
        virtual bool decodeInstruction(const BufferView &view, const InstructionPtr& instruction);
        virtual void decodeOperand(u32 opvalue, const InstructionPtr& instruction, const AVR8Operand& avrop, size_t opidx);

    private:
        std::unordered_map<u16, OpCodeCallback> m_opcodes;
};

DECLARE_ASSEMBLER_PLUGIN(AVR8Assembler, avr8, 16)

} // namespace REDasm

#endif // AVR8_H
