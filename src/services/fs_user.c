#include "util.h"
#include "handles.h"
#include "fs.h"
#include "mem.h"
#include "config.h"
#include "loader.h"

#include "service_macros.h"

/* _____ File Server Service _____ */

static u32 priority;

#ifndef DISABLE_DEBUG
static char tmp[256];
#endif


SERVICE_START(fs_user);

SERVICE_CMD(0x08010002)   // Initialize
{
    DEBUG("Initialize\n");

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08040142)   // DeleteFile
{
    u32 transaction = CMD(1);
    u32 handle_arch_lo = CMD(2);
    u32 handle_arch = CMD(3);
    u32 file_lowpath_type = CMD(4);
    u32 file_lowpath_sz = CMD(5);
    u32 transaction2 = CMD(6);
    u32 file_lowpath_ptr = CMD(7);

    DEBUG("DeleteFile\n");
    DEBUG("   archive_handle=%08x\n",
          handle_arch);
    DEBUG("   dir_lowpath_type=%s\n",
          fs_PathTypeToString(file_lowpath_type));
    DEBUG("   dir_lowpath=%s\n",
          fs_PathToString(file_lowpath_type, file_lowpath_ptr, file_lowpath_sz, tmp, sizeof(tmp)));

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }
    archive* arch = (archive*)arch_hi->subtype;
    // Call delarchive
    if (arch != NULL && arch->fnDeleteFile != NULL) {
        int ret = arch->fnDeleteFile(arch,
        (file_path) {
            file_lowpath_type, file_lowpath_sz, file_lowpath_ptr
        });

        if (ret != 0) {
            ERROR("DeleteFile has failed. --emulate it worked but it did not so the code will generate the files.--\n");
            RESP(1, 0);//RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented DeleteFile.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08050244)   // RenameFile
{
    u32 transaction = CMD(1);
    u32 src_handle_arch_lo = CMD(2);
    u32 src_handle_arch = CMD(3);
    u32 src_file_lowpath_type = CMD(4);
    u32 src_file_lowpath_sz = CMD(5);
    u32 dst_handle_arch_lo = CMD(6);
    u32 dst_handle_arch = CMD(7);
    u32 dst_file_lowpath_type = CMD(8);
    u32 dst_file_lowpath_sz = CMD(9);
    u32 transaction2 = CMD(10);
    u32 src_file_lowpath_ptr = CMD(11);
    u32 transaction3 = CMD(12);
    u32 dst_file_lowpath_ptr = CMD(13);

    DEBUG("RenameFile\n");
    DEBUG("   src_archive_handle=%08x\n",
          src_handle_arch);
    DEBUG("   src_dir_lowpath_type=%s\n",
          fs_PathTypeToString(src_file_lowpath_type));
    DEBUG("   src_dir_lowpath=%s\n",
          fs_PathToString(src_file_lowpath_type, src_file_lowpath_ptr, src_file_lowpath_sz, tmp, sizeof(tmp)));
    DEBUG("   dst_archive_handle=%08x\n",
          dst_handle_arch);
    DEBUG("   dst_dir_lowpath_type=%s\n",
          fs_PathTypeToString(dst_file_lowpath_type));
    DEBUG("   dst_dir_lowpath=%s\n",
          fs_PathToString(dst_file_lowpath_type, dst_file_lowpath_ptr, dst_file_lowpath_sz, tmp, sizeof(tmp)));

    if (src_handle_arch != dst_handle_arch) {
        ERROR("RenameFile between archives is not supported.\n");
        RESP(1, -1);
        return 0;
    }

    handleinfo* arch_hi = handle_Get(src_handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }
    archive* arch = (archive*)arch_hi->subtype;
    // Call delarchive
    if (arch != NULL && arch->fnRenameFile != NULL) {
        int ret = arch->fnRenameFile(arch,
        (file_path) {
            src_file_lowpath_type, src_file_lowpath_sz, src_file_lowpath_ptr
        },
        (file_path) {
            dst_file_lowpath_type, dst_file_lowpath_sz, dst_file_lowpath_ptr
        }
                                    );

        if (ret == EACCES) {
            ERROR("RenameFile has failed. Destination file already exists.\n");
            RESP(1, 0xC82044BE);
            return 0;
        } else if (ret == ENOENT) {
            ERROR("RenameFile has failed. Source file doesn't exist or is a directory.\n");
            RESP(1, 0xC8804478);
            return 0;
        } else if (ret != 0) {
            ERROR("RenameFile has failed.--\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented RenameFile.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08060142)   // DeleteDirectory
{
    u32 transaction = CMD(1);
    u32 handle_arch_lo = CMD(2);
    u32 handle_arch = CMD(3);
    u32 dir_lowpath_type = CMD(4);
    u32 dir_lowpath_sz = CMD(5);
    u32 transaction2 = CMD(6);
    u32 dir_lowpath_ptr = CMD(7);

    DEBUG("DeleteDirectory\n");
    DEBUG("   archive_handle=%08x\n",
          handle_arch);
    DEBUG("   dir_lowpath_type=%s\n",
          fs_PathTypeToString(dir_lowpath_type));
    DEBUG("   dir_lowpath=%s\n",
          fs_PathToString(dir_lowpath_type, dir_lowpath_ptr, dir_lowpath_sz, tmp, sizeof(tmp)));

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }
    archive* arch = (archive*)arch_hi->subtype;
    // Call delarchive
    if (arch != NULL && arch->fnDeleteDir != NULL) {
        int ret = arch->fnDeleteDir(arch,
        (file_path) {
            dir_lowpath_type, dir_lowpath_sz, dir_lowpath_ptr
        });

        if (ret != 0) {
            ERROR("DeleteDirectory has failed.\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented DeleteDirectory.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08070142)   // DeleteDirectoryRecursively
{
    u32 transaction = CMD(1);
    u32 handle_arch_lo = CMD(2);
    u32 handle_arch = CMD(3);
    u32 dir_lowpath_type = CMD(4);
    u32 dir_lowpath_sz = CMD(5);
    u32 transaction2 = CMD(6);
    u32 dir_lowpath_ptr = CMD(7);

    DEBUG("DeleteDirectoryRecursively\n");
    DEBUG("   archive_handle=%08x\n",
          handle_arch);
    DEBUG("   dir_lowpath_type=%s\n",
          fs_PathTypeToString(dir_lowpath_type));
    DEBUG("   dir_lowpath=%s\n",
          fs_PathToString(dir_lowpath_type, dir_lowpath_ptr, dir_lowpath_sz, tmp, sizeof(tmp)));

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }
    archive* arch = (archive*)arch_hi->subtype;
    // Call delarchive
    if (arch != NULL && arch->fnDeleteDir != NULL) {
        int ret = arch->fnDeleteDir(arch,
        (file_path) {
            dir_lowpath_type, dir_lowpath_sz, dir_lowpath_ptr
        });

        if (ret != 0) {
            ERROR("DeleteDirectoryRecursively has failed.\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented DeleteDirectoryRecursively.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08090182)   // CreateDirectory
{
    u32 transaction = CMD(1);
    u32 handle_arch_lo = CMD(2);
    u32 handle_arch = CMD(3);
    u32 dir_lowpath_type = CMD(4);
    u32 dir_lowpath_sz = CMD(5);
    u32 flags = CMD(6);
    u32 transaction2 = CMD(7);
    u32 dir_lowpath_ptr = CMD(8);


    DEBUG("CreateDirectory\n");
    DEBUG("   archive_handle=%08x\n",
          handle_arch);
    DEBUG("   dir_lowpath_type=%s\n",
          fs_PathTypeToString(dir_lowpath_type));
    DEBUG("   dir_lowpath=%s\n",
          fs_PathToString(dir_lowpath_type, dir_lowpath_ptr, dir_lowpath_sz, tmp, sizeof(tmp)));

    DEBUG("   dir_flags=%08x\n", flags);

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }
    archive* arch = (archive*)arch_hi->subtype;
    // Call delarchive
    if (arch != NULL && arch->fnCreateDir != NULL) {
        int ret = arch->fnCreateDir(arch,
        (file_path) {
            dir_lowpath_type, dir_lowpath_sz, dir_lowpath_ptr
        });

        if (ret != 0) {
            ERROR("CreateDir has failed.\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented CreateDir.\n");
        RESP(1, -1);
        return 0;
    }

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x080A0244)   // RenameDirectory
{
    u32 transaction = CMD(1);
    u32 src_handle_arch_lo = CMD(2);
    u32 src_handle_arch = CMD(3);
    u32 src_file_lowpath_type = CMD(4);
    u32 src_file_lowpath_sz = CMD(5);
    u32 dst_handle_arch_lo = CMD(6);
    u32 dst_handle_arch = CMD(7);
    u32 dst_file_lowpath_type = CMD(8);
    u32 dst_file_lowpath_sz = CMD(9);
    u32 transaction2 = CMD(10);
    u32 src_file_lowpath_ptr = CMD(11);
    u32 transaction3 = CMD(12);
    u32 dst_file_lowpath_ptr = CMD(13);

    DEBUG("RenameDirectory\n");
    DEBUG("   src_archive_handle=%08x\n",
          src_handle_arch);
    DEBUG("   src_dir_lowpath_type=%s\n",
          fs_PathTypeToString(src_file_lowpath_type));
    DEBUG("   src_dir_lowpath=%s\n",
          fs_PathToString(src_file_lowpath_type, src_file_lowpath_ptr, src_file_lowpath_sz, tmp, sizeof(tmp)));
    DEBUG("   dst_archive_handle=%08x\n",
          dst_handle_arch);
    DEBUG("   dst_dir_lowpath_type=%s\n",
          fs_PathTypeToString(dst_file_lowpath_type));
    DEBUG("   dst_dir_lowpath=%s\n",
          fs_PathToString(dst_file_lowpath_type, dst_file_lowpath_ptr, dst_file_lowpath_sz, tmp, sizeof(tmp)));

    if (src_handle_arch != dst_handle_arch) {
        ERROR("RenameDirectory between archives is not supported.\n");
        RESP(1, -1);
        return 0;
    }

    handleinfo* arch_hi = handle_Get(src_handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }
    archive* arch = (archive*)arch_hi->subtype;
    // Call delarchive
    if (arch != NULL && arch->fnRenameDir != NULL) {
        int ret = arch->fnRenameDir(arch,
        (file_path) {
            src_file_lowpath_type, src_file_lowpath_sz, src_file_lowpath_ptr
        },
        (file_path) {
            dst_file_lowpath_type, dst_file_lowpath_sz, dst_file_lowpath_ptr
        }
                                   );

        if (ret == EACCES) {
            ERROR("RenameDirectory has failed. Destination folder already exists.\n");
            RESP(1, 0xC82044BE);
            return 0;
        } else if (ret == ENOENT) {
            ERROR("RenameDirectory has failed. Source file doesn't exist or is a file.\n");
            RESP(1, 0xC8804478);
            return 0;
        } else if (ret != 0) {
            ERROR("RenameDirectory has failed.--\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented RenameDirectory.\n");
        RESP(1, -1);
        return 0;
    }

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
            arm11_Dump();
            RESP(1, 0xc8804464);
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
SERVICE_CMD(0x080b0102) //OpenDirectory
{
    u32 handle_arch_lo = CMD(1);
    u32 handle_arch = CMD(2);
    u32 dirtype = CMD(3);
    u32 size = CMD(4);
    u32 dirpointer = CMD(6);

    DEBUG("OpenDirectory\n");
    DEBUG("   archive_handle=%08x\n",
          handle_arch);
    DEBUG("   file_lowpath_type=%s\n",
          fs_PathTypeToString(dirtype));
    DEBUG("   file_lowpath=%s\n",
          fs_PathToString(dirtype, dirpointer, size, tmp, sizeof(tmp)));

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }

    archive* arch = (archive*)arch_hi->subtype;
    u32 file_handle = 0;

    // Call OpenFile
    if (arch != NULL && arch->fnOpenDir != NULL) {
        file_handle = arch->fnOpenDir(arch,
        (file_path) {
            dirtype, size,dirpointer
        });

        if (file_handle == 0) {
            ERROR("Dir has failed.\n");
            RESP(1, -1);
            return 0;
        }
    } else {
        ERROR("Archive has not implemented Dir.\n");
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
          fs_AttrToString(attr, tmp));

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
		ERROR("OpenArchive not implemented for %x\n", arch_id);
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
	DEBUG("   initial_size=0x%x\n", size);

    handleinfo* arch_hi = handle_Get(handle_arch);

    if (arch_hi == NULL) {
        ERROR("Invalid handle.\n");
        RESP(1, -1);
        return 0;
    }

    archive* arch = (archive*)arch_hi->subtype;
    u32 file_handle = 0;

    if (arch != NULL && arch->fnFileExists != NULL) {
        if (arch->fnFileExists(arch,
            (file_path) {
            file_lowpath_type, file_lowpath_sz, file_lowpath_ptr
        },
        flags, attr)) {

            RESP(1, 0x82044BE); // There is already a file or directory in the requested location
            return 0;
        }
    }

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
    } else {
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

    RESP(1, arch->result); // Result
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

    if(handle_lo == 0xDEADC0DE) {
        handleinfo* hi = handle_Get(handle_hi);

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

SERVICE_CMD(0x08170000)   // IsSdmcDetected
{
    DEBUG("IsSdmcDetected\n");

    RESP(1, 0);
    if (config_has_sdmc)
        RESP(2, 1); //true
    else
        RESP(2, 0); //false
    return 0;
}

SERVICE_CMD(0x08180000)   //IsSdmcWritable
{
    DEBUG("IsSdmcWritable\n");

    RESP(1, 0);
    if (config_sdmcwriteable)
        RESP(2, 1); //true
    else
        RESP(2, 0); //false
    return 0;
}

SERVICE_CMD(0x08210000)   // CardSlotIsInserted
{
    DEBUG("CardSlotIsInserted\n");

    RESP(1, 0);
    if (config_slotone)
        RESP(2, 1); //true
    else
        RESP(2, 0); //false
    return 0;
}

SERVICE_CMD(0x08330082)   // EnumerateExtSaveData
{
    DEBUG("EnumerateExtSaveData -- TODO --\n");

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x08490040)   // GetArchiveResource
{
    DEBUG("GetArchiveResource -- TODO --\n");

    RESP(1, -1); // Result
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

    RESP(1, 0);
    RESP(2, priority);
    return 0;
}
SERVICE_CMD(0x080D0144)   // ControlArchive
{
    DEBUG("ControlArchive %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n",CMD(1),CMD(2),CMD(3),CMD(4),CMD(5),CMD(6),CMD(7),CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x084500c2)   // GetFormatInfo
{
	DEBUG("GetFormatInfo %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

	RESP(1, 0);
	RESP(2, 255); //entrydir
	RESP(3, 255); //entryfile
	RESP(4, 0);
	RESP(5, 0);
	return 0;
}

SERVICE_CMD(0x084c0242)   // FormatSaveData
{
	DEBUG("FormatSaveData %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    char p[256];
    snprintf(p, 256, "savedata/%s", loader_h.productcode);

#ifdef _MSC_VER 
    return _mkdir(p);
#else
    return mkdir(p, 0777);
#endif

	RESP(1, 0);
	return 0;
}

SERVICE_END();



/* ____ File Service ____ */

SERVICE_START(file);

SERVICE_CMD(0x080200C2)   // Read
{
    u32 rc, read;
    u64 off = CMD(1) | (((u64) CMD(2)) << 32);
    u32 sz  = CMD(3);
    u32 ptr = CMD(5);

    DEBUG("Read, off=%llx, len=%x pointer=%08x\n", off, sz, ptr);

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

SERVICE_CMD(0x08010100)   // OpenSubFile
{

    DEBUG("OpenSubFile, %08x %08x %08x %08x --stub--\n", CMD(1), CMD(2), CMD(3), CMD(4));


    RESP(3, 0); // handle
    RESP(1, 0); // Result
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
    } else {
        ERROR("GetSize() not implemented for this type.\n");
        rc = -1;
    }

    RESP(1, rc); // Result
    RESP(2, (u32)sz);
    RESP(3, (u32)(sz >> 32));
    return 0;
}

SERVICE_CMD(0x08050080)   // SetSize
{
    u32 rc;
    u64 sz = CMD(1) | ((u64)CMD(2)) << 32;

    DEBUG("SetSize, sz=%llx(%lld)\n", sz, sz);

    file_type* type = (file_type*)h->subtype;

    if (type->fnSetSize != NULL) {
        rc = type->fnSetSize(type, sz);
    } else {
        ERROR("SetSize() not implemented for this type.\n");
        rc = -1;
    }

    RESP(1, rc); // Result
    return 0;
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
SERVICE_CMD(0x08090000)   // Flush
{
    DEBUG("Flush\n");
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x080A0040)   // SetPriority
{
    priority = CMD(1);
    DEBUG("SetPriority, prio=%x\n", priority);
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x080B0000)   // GetPriority
{
    DEBUG("GetPriority\n");

    RESP(1, 0);
    RESP(2, priority);
    return 0;
}

SERVICE_CMD(0x080C0000)   // OpenLinkFile
{
    file_type* type = (file_type*)h->subtype;

    DEBUG("OpenLinkFile\n");
    RESP(1, 0);
    RESP(3, type->handle);
    return 0;
}

SERVICE_END();

u32 file_CloseHandle(ARMul_State *state, handleinfo* h)
{
    DEBUG("file_CloseHandle - STUB\n");
    return 0;
}

SERVICE_START(dir);
SERVICE_CMD(0x08010042)   // Read
{
    u32 rc;
    u32 max_out_count = CMD(1);
    u32 outptr = CMD(3);

    u32 count_read = 0;

    DEBUG("ReadDir (%08x, %08x)\n", max_out_count, outptr);

    dir_type* type = (dir_type*)h->subtype; //(dir_type* self, u32 ptr, u32 entrycount, u32* read_out);

    if (type->fnRead != NULL) {
        rc = type->fnRead(type, outptr, max_out_count, &count_read);
    } else {
        ERROR("Dirread not implemented for this type.\n");
        rc = -1;
    }

    RESP(1, rc); // Result
    RESP(2, count_read); // count
    return 0;
}

SERVICE_CMD(0x08020000)   // Close
{
    u32 rc = 0;
    file_type* type = (file_type*)h->subtype;

    DEBUG("CloseDir\n");

    //if (type->fnDirClose != NULL)
    //    rc = type->fnDirClose(type);

    RESP(1, rc);
    return 0;
}

SERVICE_END();
