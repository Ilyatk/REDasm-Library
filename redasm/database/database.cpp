#include "database.h"
#include "../support/serializer.h"
#include "../plugins/assembler/assembler.h"
#include "../plugins/loader.h"
#include "../plugins/plugins.h"
#include <fstream>
#include <array>

namespace REDasm {

std::string Database::m_lasterror;

bool Database::save(DisassemblerAPI *disassembler, const std::string &dbfilename, const std::string& filename)
{
    m_lasterror.clear();
    std::fstream ofs(dbfilename, std::ios::out | std::ios::binary | std::ios::trunc);

    if(!ofs.is_open())
    {
        m_lasterror = "Cannot save " + REDasm::quoted(dbfilename);
        return false;
    }

    auto& document = disassembler->document();
    LoaderPlugin* loader = disassembler->loader();
    AssemblerPlugin* assembler = disassembler->assembler();
    ReferenceTable* references = disassembler->references();

    ofs.write(RDB_SIGNATURE, RDB_SIGNATURE_LENGTH);
    Serializer::serializeScalar(ofs, RDB_VERSION, sizeof(u32));
    Serializer::obfuscateString(ofs, filename);
    Serializer::serializeString(ofs, loader->id());
    Serializer::serializeString(ofs, assembler->id());

    if(!Serializer::compressBuffer(ofs, loader->buffer()))
    {
        m_lasterror = "Cannot compress database " + REDasm::quoted(dbfilename);
        return false;
    }

    document->serializeTo(ofs);
    references->serializeTo(ofs);
    return true;
}

Disassembler *Database::load(const std::string &dbfilename, std::string &filename)
{
    m_lasterror.clear();
    std::fstream ifs(dbfilename, std::ios::in | std::ios::binary);

    if(!ifs.is_open())
    {
        m_lasterror = "Cannot open " + REDasm::quoted(dbfilename);
        return nullptr;
    }

    if(!Serializer::checkSignature(ifs, RDB_SIGNATURE))
    {
        m_lasterror = "Signature check failed for " + REDasm::quoted(dbfilename);
        return nullptr;
    }

    u32 version = 0;
    Serializer::deserializeScalar(ifs, &version, sizeof(u32));

    if(version != RDB_VERSION)
    {
        m_lasterror = "Invalid version, got " + std::to_string(version) + " " + std::to_string(RDB_VERSION) + " required";
        return nullptr;
    }

    auto* buffer = new MemoryBuffer();
    std::string loaderid, assemblerid;
    Serializer::deobfuscateString(ifs, filename);
    Serializer::deserializeString(ifs, loaderid);
    Serializer::deserializeString(ifs, assemblerid);

    if(!Serializer::decompressBuffer(ifs, buffer))
    {
        m_lasterror = "Cannot decompress database " + REDasm::quoted(dbfilename);
        return nullptr;
    }

    const LoaderPlugin_Entry* loaderentry = REDasm::getLoader(loaderid);

    if(!loaderentry)
    {
        m_lasterror = "Unsupported loader: " + REDasm::quoted(loaderid);
        delete buffer;
        return nullptr;
    }

    LoadRequest request(filename, buffer);
    std::unique_ptr<LoaderPlugin> loader(loaderentry->init(request)); // LoaderPlugin takes the ownership of the buffer
    const AssemblerPlugin_Entry* assemblerentry = REDasm::getAssembler(assemblerid);

    if(!assemblerentry)
    {
        m_lasterror = "Unsupported assembler: " + REDasm::quoted(assemblerid);
        return nullptr;
    }

    auto& document = loader->createDocument(); // Discard old document
    document->deserializeFrom(ifs);

    auto* disassembler = new Disassembler(assemblerentry->init(), loader.release()); // Take ownership
    ReferenceTable* references = disassembler->references();
    references->deserializeFrom(ifs);
    return disassembler;
}

const std::string &Database::lastError() { return m_lasterror; }

} // namespace REDasm
