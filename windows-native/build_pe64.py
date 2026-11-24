#!/usr/bin/env python3
"""
Clean, deterministic Linux-to-Win64 PE32+ Cross-Linker & Resource Compiler.
Uses GCC (-mabi=ms -fpie -fno-plt) + GNU ld (elf_x86_64 at Win64 RVAs) + Python PE32+ Emitter.
Produces a 100% standalone Windows x64 GUI executable (.exe) with:
  - Valid PE32+ Optional Header, ASLR (.reloc), NX_COMPAT, Checksum
  - Clean .idata Import Directory & IAT (__imp_* symbols)
  - Embedded .rsrc Multi-Resolution Icon (RT_GROUP_ICON + RT_ICON) & HiDPI asInvoker Manifest (RT_MANIFEST)
"""

import re
import struct
import subprocess
from pathlib import Path

IMAGE_BASE = 0x140000000
SECT_ALIGN = 0x1000
FILE_ALIGN = 0x200


def align_up(val: int, align: int) -> int:
    return (val + align - 1) & ~(align - 1)


def build_idata_layout(imports: dict[str, list[str]], idata_rva: int):
    """
    Build a complete .idata section at `idata_rva`:
      1. IAT arrays (8-byte aligned, null-terminated per DLL) -> records exact RVA of each `__imp_<func>`
      2. IMAGE_IMPORT_DESCRIPTOR array ((N + 1) * 20 bytes)
      3. ILT (OriginalFirstThunk) arrays (8-byte aligned, identical initial contents to IAT)
      4. Hint/Name table (2-byte aligned {uint16 hint=0, ascii name + NUL}) & DLL ASCII names
    """
    dll_items = list(imports.items())

    total_iat_qwords = sum(len(funcs) + 1 for _, funcs in dll_items)
    iat_offset = 0
    iat_size = total_iat_qwords * 8

    desc_offset = align_up(iat_offset + iat_size, 8)
    desc_size = (len(dll_items) + 1) * 20

    ilt_offset = align_up(desc_offset + desc_size, 8)
    ilt_size = total_iat_qwords * 8

    strings_offset = align_up(ilt_offset + ilt_size, 4)

    string_pool = bytearray()
    dll_name_rvas: dict[str, int] = {}
    func_hint_rvas: dict[tuple[str, str], int] = {}

    for dll_name, funcs in dll_items:
        if len(string_pool) % 2 != 0:
            string_pool.append(0)
        dll_name_rvas[dll_name] = idata_rva + strings_offset + len(string_pool)
        string_pool.extend(dll_name.encode("ascii") + b"\x00")

        for fn in funcs:
            if len(string_pool) % 2 != 0:
                string_pool.append(0)
            func_hint_rvas[(dll_name, fn)] = idata_rva + strings_offset + len(string_pool)
            string_pool.extend(struct.pack("<H", 0))  # Hint = 0
            string_pool.extend(fn.encode("ascii") + b"\x00")

    total_len = strings_offset + len(string_pool)
    buf = bytearray(total_len)
    buf[strings_offset : strings_offset + len(string_pool)] = string_pool

    sym_rvas: dict[str, int] = {}
    cur_iat = iat_offset
    cur_ilt = ilt_offset
    cur_desc = desc_offset

    for dll_name, funcs in dll_items:
        this_iat_rva = idata_rva + cur_iat
        this_ilt_rva = idata_rva + cur_ilt

        for fn in funcs:
            hint_rva = func_hint_rvas[(dll_name, fn)]
            sym_rvas[f"__imp_{fn}"] = idata_rva + cur_iat
            struct.pack_into("<Q", buf, cur_iat, hint_rva)
            struct.pack_into("<Q", buf, cur_ilt, hint_rva)
            cur_iat += 8
            cur_ilt += 8

        struct.pack_into("<Q", buf, cur_iat, 0)
        struct.pack_into("<Q", buf, cur_ilt, 0)
        cur_iat += 8
        cur_ilt += 8

        struct.pack_into(
            "<IIIII",
            buf,
            cur_desc,
            this_ilt_rva,
            0,
            0,
            dll_name_rvas[dll_name],
            this_iat_rva,
        )
        cur_desc += 20

    return bytes(buf), sym_rvas, desc_offset, desc_size, iat_offset, iat_size


def build_rsrc_section(ico_path: Path, rsrc_rva: int) -> bytes:
    """
    Build a standard 3-level Windows PE .rsrc section containing:
      - Type 3  (RT_ICON):       IDs 1..N (raw icon frames from .ico)
      - Type 14 (RT_GROUP_ICON): ID 1     (GRPICONDIR with 14-byte GRPICONDIRENTRY records)
      - Type 24 (RT_MANIFEST):   ID 1     (UTF-8 Windows 10/11 HiDPI + asInvoker manifest)
    """
    ico_bytes = ico_path.read_bytes()
    reserved, ico_type, count = struct.unpack_from("<HHH", ico_bytes, 0)
    assert reserved == 0 and ico_type == 1 and count >= 1

    icon_blobs: list[bytes] = []
    grp_icon = bytearray(struct.pack("<HHH", 0, 1, count))
    for i in range(count):
        entry_off = 6 + i * 16
        b_w, b_h, b_colors, b_res, w_planes, w_bitcount, dw_bytes, dw_offset = struct.unpack_from(
            "<BBBBHHII", ico_bytes, entry_off
        )
        frame = ico_bytes[dw_offset : dw_offset + dw_bytes]
        icon_blobs.append(frame)
        icon_id = i + 1
        grp_icon.extend(
            struct.pack("<BBBBHHIH", b_w, b_h, b_colors, b_res, w_planes or 1, w_bitcount or 32, dw_bytes, icon_id)
        )

    manifest_xml = b"""<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0" xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
  <assemblyIdentity version="1.0.0.0" processorArchitecture="amd64" name="Local.DesktopShredder" type="win32"/>
  <description>Offline Native Desktop File Shredder</description>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <asmv3:application>
    <asmv3:windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2, PerMonitor</dpiAwareness>
    </asmv3:windowsSettings>
  </asmv3:application>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}"/>
    </application>
  </compatibility>
</assembly>"""

    total_leaves = count + 2

    def dir_hdr(num_id_entries: int) -> bytes:
        return struct.pack("<IIHHHH", 0, 0, 0, 0, 0, num_id_entries)

    def dir_entry(res_id: int, offset: int, is_subdir: bool) -> bytes:
        val = (offset | 0x80000000) if is_subdir else offset
        return struct.pack("<II", res_id, val)

    tree = bytearray()
    off_type3 = 16 + 3 * 8
    off_type14 = off_type3 + (16 + count * 8)
    off_type24 = off_type14 + (16 + 1 * 8)
    off_lang_start = off_type24 + (16 + 1 * 8)
    off_data_entries_start = off_lang_start + total_leaves * (16 + 8)

    tree.extend(dir_hdr(3))
    tree.extend(dir_entry(3, off_type3, True))
    tree.extend(dir_entry(14, off_type14, True))
    tree.extend(dir_entry(24, off_type24, True))

    tree.extend(dir_hdr(count))
    for i in range(count):
        lang_off = off_lang_start + i * 24
        tree.extend(dir_entry(i + 1, lang_off, True))

    tree.extend(dir_hdr(1))
    tree.extend(dir_entry(1, off_lang_start + count * 24, True))

    tree.extend(dir_hdr(1))
    tree.extend(dir_entry(1, off_lang_start + (count + 1) * 24, True))

    for idx in range(total_leaves):
        data_ent_off = off_data_entries_start + idx * 16
        tree.extend(dir_hdr(1))
        tree.extend(dir_entry(0x0409, data_ent_off, False))

    assert len(tree) == off_data_entries_start

    payloads = icon_blobs + [bytes(grp_icon), manifest_xml]
    raw_data_start = align_up(off_data_entries_start + total_leaves * 16, 16)
    data_entries = bytearray()
    blob_area = bytearray()

    for blob in payloads:
        while (raw_data_start + len(blob_area)) % 8 != 0:
            blob_area.append(0)
        blob_rva = rsrc_rva + raw_data_start + len(blob_area)
        data_entries.extend(struct.pack("<IIII", blob_rva, len(blob), 0, 0))
        blob_area.extend(blob)

    tree.extend(data_entries)
    while len(tree) < raw_data_start:
        tree.append(0)
    tree.extend(blob_area)
    return bytes(tree)


def build_reloc_section(abs64_rvas: list[int]) -> bytes:
    if not abs64_rvas:
        return struct.pack("<IIHH", 0x1000, 12, 0x0000, 0x0000)

    pages: dict[int, list[int]] = {}
    for rva in sorted(abs64_rvas):
        page = rva & ~0xFFF
        pages.setdefault(page, []).append(rva & 0xFFF)

    out = bytearray()
    for page, offsets in sorted(pages.items()):
        entries = [(10 << 12) | off for off in offsets]
        if len(entries) % 2 != 0:
            entries.append(0)
        block_size = 8 + len(entries) * 2
        out.extend(struct.pack("<II", page, block_size))
        for ent in entries:
            out.extend(struct.pack("<H", ent))
    return bytes(out)


def compute_pe_checksum(pe_bytes: bytearray, checksum_offset: int) -> int:
    struct.pack_into("<I", pe_bytes, checksum_offset, 0)
    data = bytes(pe_bytes)
    if len(data) % 2 != 0:
        data += b"\x00"
    total = 0
    for (word,) in struct.iter_unpack("<H", data):
        total += word
        total = (total & 0xFFFF) + (total >> 16)
    total = (total & 0xFFFF) + (total >> 16)
    return (total + len(pe_bytes)) & 0xFFFFFFFF


def compile_pe64(
    c_sources: list[Path],
    imports: dict[str, list[str]],
    ico_path: Path,
    out_exe: Path,
    work_dir: Path,
) -> None:
    work_dir.mkdir(parents=True, exist_ok=True)

    chkstk_s = work_dir / "chkstk.s"
    chkstk_s.write_text(
        """
        .text
        .globl ___chkstk_ms
        ___chkstk_ms:
            push %rcx
            push %rax
            cmp $0x1000, %rax
            lea 24(%rsp), %rcx
            jb 2f
        1:
            sub $0x1000, %rcx
            test %rcx, (%rcx)
            sub $0x1000, %rax
            cmp $0x1000, %rax
            ja 1b
        2:
            sub %rax, %rcx
            test %rcx, (%rcx)
            pop %rax
            pop %rcx
            ret
        .section .note.GNU-stack,"",@progbits
        """,
        encoding="utf-8",
    )

    obj_files: list[Path] = []
    chkstk_o = work_dir / "chkstk.o"
    subprocess.run(["gcc", "-c", str(chkstk_s), "-o", str(chkstk_o)], check=True)
    obj_files.append(chkstk_o)

    cflags = [
        "-O2",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-mabi=ms",
        "-fpie",
        "-fno-plt",
        "-fno-builtin",
        "-fno-asynchronous-unwind-tables",
        "-fno-unwind-tables",
        "-fno-stack-protector",
        "-mno-red-zone",
        "-fno-ident",
        "-ffunction-sections",
        "-fdata-sections",
    ]
    for src in c_sources:
        obj = work_dir / f"{src.stem}.o"
        subprocess.run(["gcc", *cflags, "-c", str(src), "-o", str(obj)], check=True)
        obj_files.append(obj)

    dummy_syms = "\n".join(
        f"PROVIDE({f'__imp_{fn}'} = 0x140080000);"
        for funcs in imports.values()
        for fn in funcs
    )
    pass1_ld = work_dir / "pass1.ld"
    pass1_ld.write_text(
        f"""
        ENTRY(WinMainCRTStartup)
        {dummy_syms}
        SECTIONS {{
          . = 0x{IMAGE_BASE + 0x1000:x};
          .text : ALIGN(16) {{ *(.text .text.*) }}
          . = ALIGN(0x1000);
          .rdata : ALIGN(16) {{ *(.rodata .rodata.*) }}
          . = ALIGN(0x1000);
          .data : ALIGN(16) {{ *(.data .data.*) }}
          .bss : ALIGN(16) {{ *(.bss .bss.* COMMON) }}
          /DISCARD/ : {{ *(.comment) *(.note*) *(.eh_frame*) }}
        }}
        """,
        encoding="utf-8",
    )
    pass1_elf = work_dir / "pass1.elf"
    subprocess.run(
        ["ld", "-m", "elf_x86_64", "--gc-sections", "-T", str(pass1_ld), *[str(o) for o in obj_files], "-o", str(pass1_elf)],
        check=True,
    )

    sec_hdrs = subprocess.check_output(["readelf", "-SW", str(pass1_elf)], text=True)
    sizes = {".text": 0, ".rdata": 0, ".data": 0, ".bss": 0}
    vmas = {".data": 0, ".bss": 0}
    for line in sec_hdrs.splitlines():
        for sname in list(sizes.keys()):
            if f" {sname} " in line:
                parts = line.split(sname)[1].split()
                if sname in vmas:
                    vmas[sname] = int(parts[1], 16)
                sizes[sname] = int(parts[3], 16)

    text_rva = 0x1000
    text_vsize = max(sizes[".text"], 16)
    rdata_rva = align_up(text_rva + text_vsize, SECT_ALIGN)
    rdata_vsize = max(sizes[".rdata"], 16)
    data_rva = align_up(rdata_rva + rdata_vsize, SECT_ALIGN)
    if sizes[".bss"] > 0 and vmas[".bss"] > 0:
        data_total_vsize = max((vmas[".bss"] + sizes[".bss"]) - (IMAGE_BASE + data_rva), 16)
    else:
        data_total_vsize = max(sizes[".data"], 16)
    idata_rva = align_up(data_rva + data_total_vsize, SECT_ALIGN)

    idata_bytes, sym_rvas, imp_desc_off, imp_desc_size, iat_off, iat_size = build_idata_layout(imports, idata_rva)

    real_syms = "\n".join(f"{sym} = 0x{IMAGE_BASE + rva:x};" for sym, rva in sym_rvas.items())
    pass2_ld = work_dir / "pass2.ld"
    pass2_ld.write_text(
        f"""
        ENTRY(WinMainCRTStartup)
        {real_syms}
        SECTIONS {{
          . = 0x{IMAGE_BASE + text_rva:x};
          .text : ALIGN(16) {{ *(.text .text.*) }}
          . = 0x{IMAGE_BASE + rdata_rva:x};
          .rdata : ALIGN(16) {{ *(.rodata .rodata.*) }}
          . = 0x{IMAGE_BASE + data_rva:x};
          .data : ALIGN(16) {{ *(.data .data.*) }}
          .bss : ALIGN(16) {{ *(.bss .bss.* COMMON) }}
          /DISCARD/ : {{ *(.comment) *(.note*) *(.eh_frame*) }}
        }}
        """,
        encoding="utf-8",
    )
    final_elf = work_dir / "final.elf"
    subprocess.run(
        [
            "ld",
            "-m",
            "elf_x86_64",
            "--gc-sections",
            "--emit-relocs",
            "-T",
            str(pass2_ld),
            *[str(o) for o in obj_files],
            "-o",
            str(final_elf),
        ],
        check=True,
    )

    def dump_sec(sec_name: str) -> bytes:
        tmp_bin = work_dir / f"sec{sec_name}.bin"
        if tmp_bin.exists():
            tmp_bin.unlink()
        res = subprocess.run(["objcopy", "-O", "binary", "--only-section=" + sec_name, str(final_elf), str(tmp_bin)])
        if res.returncode != 0 or not tmp_bin.exists():
            return b""
        return tmp_bin.read_bytes()

    text_raw = dump_sec(".text")
    rdata_raw = dump_sec(".rdata") or b"\x00" * 16
    data_init_raw = dump_sec(".data")

    assert align_up(text_rva + len(text_raw), SECT_ALIGN) == rdata_rva
    assert align_up(rdata_rva + len(rdata_raw), SECT_ALIGN) == data_rva

    elf_h = subprocess.check_output(["readelf", "-hW", str(final_elf)], text=True)
    m_ep = re.search(r"Entry point address:\s+0x([0-9a-fA-F]+)", elf_h)
    assert m_ep, "Could not parse ELF entry point"
    entry_rva = int(m_ep.group(1), 16) - IMAGE_BASE

    sec_hdrs2 = subprocess.check_output(["readelf", "-SW", str(final_elf)], text=True)
    bss_size = 0
    data_vma_end = IMAGE_BASE + data_rva + len(data_init_raw)
    for line in sec_hdrs2.splitlines():
        if " .bss " in line:
            parts = line.split(".bss")[1].split()
            bss_vma = int(parts[1], 16)
            bss_sz = int(parts[3], 16)
            data_vma_end = max(data_vma_end, bss_vma + bss_sz)
            bss_size = bss_sz

    data_vsize = max(data_vma_end - (IMAGE_BASE + data_rva), 16)
    assert align_up(data_rva + data_vsize, SECT_ALIGN) == idata_rva

    reloc_out = subprocess.check_output(["readelf", "-rW", str(final_elf)], text=True)
    abs64_rvas: list[int] = []
    for line in reloc_out.splitlines():
        if "R_X86_64_64" in line:
            vma = int(line.split()[0], 16)
            abs64_rvas.append(vma - IMAGE_BASE)

    rsrc_rva = align_up(idata_rva + len(idata_bytes), SECT_ALIGN)
    rsrc_bytes = build_rsrc_section(ico_path, rsrc_rva)

    reloc_rva = align_up(rsrc_rva + len(rsrc_bytes), SECT_ALIGN)
    reloc_bytes = build_reloc_section(abs64_rvas)

    size_of_image = align_up(reloc_rva + len(reloc_bytes), SECT_ALIGN)

    sections_spec = [
        (b".text\x00\x00\x00", len(text_raw), text_rva, text_raw, 0x60000020),
        (b".rdata\x00\x00", len(rdata_raw), rdata_rva, rdata_raw, 0x40000040),
        (b".data\x00\x00\x00", data_vsize, data_rva, data_init_raw, 0xC0000040),
        (b".idata\x00\x00", len(idata_bytes), idata_rva, idata_bytes, 0xC0000040),
        (b".rsrc\x00\x00\x00", len(rsrc_bytes), rsrc_rva, rsrc_bytes, 0x40000040),
        (b".reloc\x00\x00", len(reloc_bytes), reloc_rva, reloc_bytes, 0x42000040),
    ]

    size_of_headers = 0x400
    pe = bytearray(size_of_headers)

    struct.pack_into("<H", pe, 0x00, 0x5A4D)
    struct.pack_into("<I", pe, 0x3C, 0x80)

    nt_off = 0x80
    struct.pack_into("<I", pe, nt_off, 0x00004550)
    struct.pack_into(
        "<HHIIIHH",
        pe,
        nt_off + 4,
        0x8664,
        len(sections_spec),
        0,
        0,
        0,
        240,
        0x0022,
    )

    opt_off = nt_off + 24
    code_padded = align_up(len(text_raw), FILE_ALIGN)
    init_data_padded = sum(align_up(len(raw), FILE_ALIGN) for _, _, _, raw, _ in sections_spec[1:])

    struct.pack_into(
        "<HBBIIIIIQIIHHHHHHIIIIHHQQQQII",
        pe,
        opt_off,
        0x020B,
        14,
        0,
        code_padded,
        init_data_padded,
        bss_size,
        entry_rva,
        text_rva,
        IMAGE_BASE,
        SECT_ALIGN,
        FILE_ALIGN,
        6,
        0,
        1,
        0,
        6,
        0,
        0,
        size_of_image,
        size_of_headers,
        0,
        2,
        0x8160,
        0x200000,
        0x10000,
        0x100000,
        0x4000,
        0,
        16,
    )

    dd_off = opt_off + 112
    struct.pack_into("<II", pe, dd_off + 1 * 8, idata_rva + imp_desc_off, imp_desc_size)
    struct.pack_into("<II", pe, dd_off + 2 * 8, rsrc_rva, len(rsrc_bytes))
    struct.pack_into("<II", pe, dd_off + 5 * 8, reloc_rva, len(reloc_bytes))
    struct.pack_into("<II", pe, dd_off + 12 * 8, idata_rva + iat_off, iat_size)

    sh_off = opt_off + 240
    cur_file_ptr = size_of_headers
    section_blobs = bytearray()

    for idx, (name8, vsize, rva, raw, chars) in enumerate(sections_spec):
        raw_padded_len = align_up(len(raw), FILE_ALIGN) if len(raw) > 0 else 0
        ptr_raw = cur_file_ptr if raw_padded_len > 0 else 0

        hdr_pos = sh_off + idx * 40
        pe[hdr_pos : hdr_pos + 8] = name8
        struct.pack_into(
            "<IIIIIIHHI",
            pe,
            hdr_pos + 8,
            vsize,
            rva,
            raw_padded_len,
            ptr_raw,
            0,
            0,
            0,
            0,
            chars,
        )

        if raw_padded_len > 0:
            section_blobs.extend(raw)
            section_blobs.extend(b"\x00" * (raw_padded_len - len(raw)))
            cur_file_ptr += raw_padded_len

    pe.extend(section_blobs)

    checksum_pos = opt_off + 64
    csum = compute_pe_checksum(pe, checksum_pos)
    struct.pack_into("<I", pe, checksum_pos, csum)

    out_exe.parent.mkdir(parents=True, exist_ok=True)
    out_exe.write_bytes(pe)
    print(f"[OK] Built native Win64 GUI executable: {out_exe} ({len(pe)} bytes, CheckSum=0x{csum:08X})")
