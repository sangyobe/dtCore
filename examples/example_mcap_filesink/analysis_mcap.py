#!/usr/bin/env python3

import struct
import argparse

MAGIC = bytes([0x89, 0x4D, 0x43, 0x41, 0x50, 0x30, 0x0D, 0x0A])
RECORD_NAMES = {
    0x01: 'Header',       0x02: 'Footer',          0x03: 'Schema',
    0x04: 'Channel',      0x05: 'Message',          0x06: 'Chunk',
    0x07: 'MessageIndex', 0x08: 'ChunkIndex',       0x09: 'Attachment',
    0x0A: 'AttachmentIndex', 0x0B: 'Statistics',    0x0C: 'Metadata',
    0x0D: 'MetadataIndex',   0x0E: 'SummaryOffset', 0x0F: 'DataEnd',
}

def parse_subfile(data, start, sub_idx):
    """Parse one MCAP sub-file starting at `start` (after its leading Magic).
    Returns (next_offset, record_count, chunk_count, ok).
    """
    file_size = len(data)
    i = start
    record_count = 0
    chunk_count = 0
    found_trailing = False

    while i < file_size:
        # Trailing magic = end of this sub-file
        if i + 8 <= file_size and data[i:i+8] == MAGIC:
            print(f'  [Trailing Magic] offset=0x{i:X} → Sub-file #{sub_idx} 정상 종료')
            i += 8
            found_trailing = True
            break

        if i + 9 > file_size:
            print(f'  [Truncated] offset=0x{i:X}, 잔여 {file_size - i} bytes')
            i = file_size
            break

        op = data[i]
        if op not in RECORD_NAMES:
            print(f'  [UNKNOWN op=0x{op:02X}] offset=0x{i:X} ← 손상 지점!')
            print(f'    주변 bytes: {data[i:i+32].hex()}')
            i = file_size
            break

        length = struct.unpack_from('<Q', data, i + 1)[0]
        end = i + 1 + 8 + length
        name = RECORD_NAMES[op]

        if op == 0x06:  # Chunk
            chunk_count += 1
            if end > file_size:
                avail = file_size - i - 9
                print(f'  [Chunk #{chunk_count}] offset=0x{i:X}, length={length} → TRUNCATED! '
                      f'(선언={length}, 실제={avail}, 손실={length - avail})')
                i = file_size
                break
            print(f'  [Chunk #{chunk_count}] offset=0x{i:X}, '
                  f'length={length} ({length / 1024:.1f} KB), end=0x{end:X} → OK')
        elif op == 0x0F:  # DataEnd
            print(f'  [DataEnd] offset=0x{i:X} ✅')
        elif op == 0x02:  # Footer
            summary_start        = struct.unpack_from('<Q', data, i + 9)[0]
            summary_offset_start = struct.unpack_from('<Q', data, i + 17)[0]
            print(f'  [Footer] offset=0x{i:X}, '
                  f'summaryStart=0x{summary_start:X}, '
                  f'summaryOffsetStart=0x{summary_offset_start:X} ✅')
        else:
            print(f'  [{name}] offset=0x{i:X}, length={length}')

        if end > file_size:
            print(f'    → 레코드 크기 파일 초과! 손상됨')
            i = file_size
            break

        i = end
        record_count += 1

    if not found_trailing and i >= file_size:
        print(f'  [Warning] Trailing Magic 없음 — 파일이 잘렸을 수 있음')

    return i, record_count, chunk_count, found_trailing


def parse_mcap(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    file_size = len(data)
    print(f'File : {filepath}')
    print(f'Size : {file_size} bytes (0x{file_size:X})')
    print()

    total_records = 0
    total_chunks  = 0
    sub_idx       = 0
    i             = 0

    while i < file_size:
        if i + 8 > file_size:
            print(f'[EOF] offset=0x{i:X}, 잔여 {file_size - i} bytes (Magic 미만)')
            break

        if data[i:i+8] != MAGIC:
            print(f'[ERROR] offset=0x{i:X}: MCAP Magic 불일치: {data[i:i+8].hex()}')
            break

        sub_idx += 1
        print(f'{"=" * 60}')
        print(f'[Sub-file #{sub_idx}] starts at offset 0x{i:X}')
        print(f'  [Leading Magic] {data[i:i+8].hex()} → OK')
        i += 8  # consume leading magic

        i, record_count, chunk_count, _ = parse_subfile(data, i, sub_idx)

        total_records += record_count
        total_chunks  += chunk_count
        print(f'  → {record_count}개 레코드, {chunk_count}개 청크')
        print()

    print(f'{"=" * 60}')
    if sub_idx > 1:
        print(f'[Concatenated MCAP] {sub_idx}개 sub-file 감지됨')
    print(f'총계: {sub_idx}개 sub-file, {total_records}개 레코드, {total_chunks}개 청크')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='MCAP 파일 분석기 (concatenated MCAP 지원)')
    parser.add_argument('filepath', nargs='?', default='data/robot_state.mcap',
                        help='분석할 MCAP 파일 경로 (기본값: data/robot_state.mcap)')
    args = parser.parse_args()
    parse_mcap(args.filepath)
