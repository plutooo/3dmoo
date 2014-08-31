#include "util.h"
#include "handles.h"
#include "fs.h"
#include "mem.h"

#include "service_macros.h"

/* _____ File Server Service _____ */

static u32 priority;


SERVICE_START(fs_user);

SERVICE_CMD(0x08010002)   // Initialize
{
    DEBUG("Initialize\n");

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x080201C2)   // OpenFile
{
    u32 transaction       = CMD(1);
    u32 handle_arch_lo    = CMD(2);
    u32 handle_arch       = CMD(3);
    u32 file_lowpath_type = CMD(4);
    u32 file_lowpath_sz   = CMD(5);
    u32 flags             = CMD(6);
    u32 attr              = CMD(7);
    u32 file_lowpath_desc = CMD(8);
    u32 file_lowpath_ptr  = CMD(9);

    char tmp[256];

    DEBUG("OpenFile\n");
    DEBUG("   archive_handle=%08x\n",
          handle_arch);
    DEBUG("   flags=%s\n",
          fs_FlagsToString(flags, tmp));
    DEBUG("   file_lowpath_type=%s\n",
          fs_PathTypeToString(file_lowpath_type));
    DEBUG("   file_lowpath=%s\n",
          fs_PathToString(file_lowpath_type, file_lowpath_ptr, file_lowpath_sz, tmp, sizeof(tmp)));
    DEBUG("   attr=%s\n",
          fs_AttrToString(attr, tmp));

    handleinfo* arch_hi = handle_Get(handle_arch);

    if(arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }

    archive* arch = (archive*) arch_hi->subtype;
    u32 file_handle = 0;

    // Call OpenFile
    if(arch != NULL && arch->fnOpenFile != NULL) {
        file_handle = arch->fnOpenFile(arch,
        (file_path) {
            file_lowpath_type, file_lowpath_sz, file_lowpath_ptr
        },
        flags, attr);

        if(file_handle == 0) {
            ERROR("OpenFile has failed.\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented OpenFile.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    RESP(3, file_handle); // File handle
    return 0;
}

SERVICE_CMD(0x08030204)   // OpenFileDirectly
{
    u32 transaction       = CMD(1);
    u32 arch_id           = CMD(2);
    u32 arch_lowpath_type = CMD(3);
    u32 arch_lowpath_sz   = CMD(4);
    u32 file_lowpath_type = CMD(5);
    u32 file_lowpath_sz   = CMD(6);
    u32 flags             = CMD(7);
    u32 attr              = CMD(8);
    u32 arch_lowpath_desc = CMD(9);
    u32 arch_lowpath_ptr  = CMD(10);
    u32 file_lowpath_desc = CMD(11);
    u32 file_lowpath_ptr  = CMD(12);

    char tmp[256];
    archive_type* arch_type = fs_GetArchiveTypeById(arch_id);

    DEBUG("OpenFileDirectly\n");
    DEBUG("   archive_id=%x (%s)\n",
          arch_id, arch_type == NULL ? "unknown" : arch_type->name);
    DEBUG("   flags=%s\n",
          fs_FlagsToString(flags, tmp));
    DEBUG("   arch_lowpath_type=%s\n",
          fs_PathTypeToString(arch_lowpath_type));
    DEBUG("   arch_lowpath=%s\n",
          fs_PathToString(arch_lowpath_type, arch_lowpath_ptr, arch_lowpath_sz, tmp, sizeof(tmp)));
    DEBUG("   file_lowpath_type=%s\n",
          fs_PathTypeToString(file_lowpath_type));
    DEBUG("   file_lowpath=%s\n",
          fs_PathToString(file_lowpath_type, file_lowpath_ptr, file_lowpath_sz, tmp, sizeof(tmp)));
    DEBUG("   attr=%s\n",
          fs_AttrToString(flags, tmp));

    // Call OpenArchive
    if(arch_type != NULL && arch_type->fnOpenArchive != NULL) {
        archive* arch = arch_type->fnOpenArchive(
        (file_path) {
            arch_lowpath_type, arch_lowpath_sz, arch_lowpath_ptr
        });

        if(arch != NULL) {

            // Call OpenFile
            if(arch->fnOpenFile != NULL) {
                u32 file_handle = arch->fnOpenFile(arch,
                (file_path) {
                    file_lowpath_type, file_lowpath_sz, file_lowpath_ptr
                },
                flags, attr);

                RESP(1, 0); // Result
                RESP(3, file_handle); // File handle
            } else {
                ERROR("OpenFile not implemented for %x\n", arch_id);
                RESP(1, -1);
            }

            if(arch->fnDeinitialize != NULL)
                arch->fnDeinitialize(arch);
        } else {
            ERROR("OpenArchive failed\n");
            RESP(1, -1);
        }
    } else {
        ERROR("OpenArchive not implemented for %x\n", arch_id)
        RESP(1, -1);
    }

    return 0;
}

SERVICE_CMD(0x08080202)   // CreateFile
{
    u32 transaction = CMD(1);
    u32 handle_arch_lo = CMD(2);
    u32 handle_arch = CMD(3);
    u32 file_lowpath_type = CMD(4);
    u32 file_lowpath_sz = CMD(5);
    u32 unk0 = CMD(6);
    u32 size = CMD(7);
    u32 unk1 = CMD(8);
    u32 unk2 = CMD(9);
    u32 file_lowpath_ptr = CMD(10);
    u32 unk3 = CMD(11);
    u32 unk4 = CMD(12);

    u32 flags = 6; //Create
    u32 attr = 0;

    char tmp[256];

    DEBUG("CreateFile\n");
    DEBUG("   archive_handle=%08x\n",
        handle_arch);
    DEBUG("   flags=%s\n",
        fs_FlagsToString(flags, tmp));
    DEBUG("   file_lowpath_type=%s\n",
        fs_PathTypeToString(file_lowpath_type));
    DEBUG("   file_lowpath=%s\n",
        fs_PathToString(file_lowpath_type, file_lowpath_ptr, file_lowpath_sz, tmp, sizeof(tmp)));
    DEBUG("   attr=%s\n",
        fs_AttrToString(attr, tmp));

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }

    archive* arch = (archive*)arch_hi->subtype;
    u32 file_handle = 0;

    // Call OpenFile
    if (arch != NULL && arch->fnOpenFile != NULL) {
        file_handle = arch->fnOpenFile(arch,
            (file_path) {
            file_lowpath_type, file_lowpath_sz, file_lowpath_ptr
        },
        flags, attr);

            if (file_handle == 0) {
                ERROR("CreateFile has failed.\n");
                RESP(1, -1);
                return 0;
            }
    }
    else {
        ERROR("Archive has not implemented CreateFile/OpenFile.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    RESP(3, file_handle); // File handle
    return 0;
}

SERVICE_CMD(0x080C00C2)   // OpenArchive
{
    u32 arch_id           = CMD(1);
    u32 arch_lowpath_type = CMD(2);
    u32 arch_lowpath_sz   = CMD(3);
    u32 arch_lowpath_desc = CMD(4);
    u32 arch_lowpath_ptr  = CMD(5);

    char tmp[256];
    archive_type* arch_type = fs_GetArchiveTypeById(arch_id);

    DEBUG("OpenArchive\n");
    DEBUG("   archive_id=%x (%s)\n",
          arch_id, arch_type == NULL ? "unknown" : arch_type->name);
    DEBUG("   arch_lowpath_type=%s\n",
          fs_PathTypeToString(arch_lowpath_type));
    DEBUG("   arch_lowpath=%s\n",
          fs_PathToString(arch_lowpath_type, arch_lowpath_ptr, arch_lowpath_sz, tmp, sizeof(tmp)));

    if(arch_type == NULL) {
        ERROR("Unknown archive type: %x.\n", arch_id);
        RESP(1, -1);
        return 0;
    }

    if(arch_type->fnOpenArchive == NULL) {
        ERROR("Archive type %x has not implemented OpenArchive\n", arch_id);
        RESP(1, -1);
        return 0;
    }

    archive* arch = arch_type->fnOpenArchive(
    (file_path) {
        arch_lowpath_type, arch_lowpath_sz, arch_lowpath_ptr
    });

    if(arch == NULL) {
        ERROR("OpenArchive failed\n");
        RESP(1, -1);
        return 0;
    }

    u32 arch_handle = handle_New(HANDLE_TYPE_ARCHIVE, (uintptr_t) arch);

    RESP(1, 0); // Result
    RESP(2, 0xdeadc0de); // Arch handle lo (not used)
    RESP(3, arch_handle);  // Arch handle hi
    return 0;
}

SERVICE_CMD(0x080E0080)   // CloseArchive
{
    DEBUG("CloseArchive\n");

    u32 handle_lo = CMD(1);
    u32 handle_hi = CMD(2);
    u32 rc = -1;

    if(handle_hi == 0xDEADC0DE) {
        handleinfo* hi = handle_Get(handle_lo);

        if(hi != NULL && hi->type == HANDLE_TYPE_ARCHIVE) {
            archive* arch = (archive*) hi->subtype;

            if(arch->fnDeinitialize != NULL)
                arch->fnDeinitialize(arch);

            rc = 0;
        }
    }

    if(rc == -1) {
        ERROR("Invalid handle.\n");
    }

    RESP(1, rc);
    return 0;
}

SERVICE_CMD(0x08610042)   // InitializeWithSdkVersion
{
    DEBUG("InitializeWithSdkVersion -- TODO --\n");

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08620040)   // SetPriority
{
    priority = CMD(1);
    DEBUG("SetPriority, prio=%x\n", priority);

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x08630000)   // GetPriority
{
    DEBUG("GetPriority\n");

    RESP(1, priority);
    return 0;
}

SERVICE_CMD(0x08170000)   // IsSdmcDetected
{
    DEBUG("IsSdmcDetected - STUBBED -\n");

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x08210000)   // CardSlotIsInserted
{
    DEBUG("CardSlotIsInserted - STUBBED -\n");

    RESP(1, 0);
    return 0;
}


SERVICE_END();



/* ____ File Service ____ */

SERVICE_START(file);

SERVICE_CMD(0x080200C2)   // Read
{
    u32 rc, read;
    u64 off = CMD(1) | ((u64) CMD(2)) << 32;
    u32 sz  = CMD(3);
    u32 ptr = CMD(5);

    DEBUG("Read, off=%llx, len=%x\n", off, sz);

    file_type* type = (file_type*) h->subtype;

    if(type->fnRead != NULL) {
        rc = type->fnRead(type, ptr, sz, off, &read);
    } else {
        ERROR("Read() not implemented for this type.\n");
        rc = -1;
        read = 0;
    }

    RESP(1, rc); // Result
    RESP(2, read);
    return 0;
}

SERVICE_CMD(0x08030102)   // Write
{
    u32 rc, written=0;
    u64 off = CMD(1) | ((u64) CMD(2)) << 32;
    u32 sz  = CMD(3);
    u32 flush_flags = CMD(4);
    u32 ptr = CMD(6);

    DEBUG("Write, off=%llx, len=%x, flush=%x\n", off, sz, flush_flags);

    file_type* type = (file_type*) h->subtype;

    if(type->fnWrite != NULL) {
        rc = type->fnWrite(type, ptr, sz, off, flush_flags, &written);
    } else {
        ERROR("Write() not implemented for this type.\n");
        rc = -1;
        written = 0;
    }

    RESP(1, rc); // Result
    RESP(2, written);
    return 0;
}

SERVICE_CMD(0x08040000)   // GetSize
{
    u32 rc = 0;
    u64 sz = 0;

    DEBUG("GetSize\n");

    file_type* type = (file_type*)h->subtype;

    if (type->fnGetSize != NULL) {
        sz = type->fnGetSize(type);
    }
    else {
        ERROR("GetSize() not implemented for this type.\n");
        rc = -1;
    }

    RESP(1, rc); // Result
    RESP(2, (u32)(sz >> 32));
    RESP(3, (u32)sz);
    return rc;
}

SERVICE_CMD(0x08050080)   // SetSize
{
    u32 rc;
    u64 sz = CMD(1) | ((u64)CMD(2)) << 32;

    DEBUG("SetSize, sz=%llx(%lld)\n", sz, sz);

    file_type* type = (file_type*)h->subtype;

    if (type->fnSetSize != NULL) {
        rc = type->fnSetSize(type, sz);
    }
    else {
        ERROR("SetSize() not implemented for this type.\n");
        rc = -1;
    }

    RESP(1, rc); // Result
    return rc;
}

SERVICE_CMD(0x08080000)   // Close
{
    u32 rc = 0;
    file_type* type = (file_type*) h->subtype;

    DEBUG("Close\n");

    if(type->fnClose != NULL)
        rc = type->fnClose(type);

    RESP(1, rc);
    return 0;
}

SERVICE_END();

u32 file_CloseHandle(ARMul_State *state, handleinfo* h)
{
    DEBUG("file_CloseHandle - STUB\n");
    return 0;
}
