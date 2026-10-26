"""Find retained RIP-relative MOV/LEA references, including non-REX DWORD loads."""
import re
import struct


def mapping_references(image, targets):
    found = set()
    pattern = rb'(?:[\x48-\x4f])?[\x8d\x8b\x89][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]'
    for match in re.finditer(pattern, image.code):
        offset = match.start()
        size = len(match.group())+4
        if offset+size > len(image.code) or not all(image.valid[offset:offset+size]):
            continue
        address = image.base+offset
        target = address+size+struct.unpack_from('<i', image.code, offset+size-4)[0]
        if target not in targets:
            continue
        instructions = image.instructions(address)
        if any(i.address == address and i.size == size and image.target(i) == target for i in instructions):
            found.add((address, target))
    return sorted(found)
