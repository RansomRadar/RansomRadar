use crate::header::*;
use core::alloc::{GlobalAlloc, Layout};

extern "system" {
    pub fn ExAllocatePool2(Flags: POOL_FLAGS, NumberOfBytes: SIZE_T, Tag: ULONG) -> PVOID;
    pub fn ExFreePool(P: PVOID);
}

#[alloc_error_handler]
fn alloc_error(_: Layout) -> ! {
    loop {}
}

/// Represents a kernel allocator that relies on the `ExAllocatePool` family of functions to
/// allocate and free memory for the `alloc` crate.
pub struct KernelAllocator {
    /// The 32-bit tag to use for the pool, this is usually derived from a quadruplet of ASCII
    /// bytes, e.g. by invoking `u32::from_ne_bytes(*b"rust")`.
    tag: u32,
}

impl KernelAllocator {
    /// Sets up a new kernel allocator with the 32-bit tag specified. The tag is usually derived
    /// from a quadruplet of ASCII bytes, e.g. by invoking `u32::from_ne_bytes(*b"rust")`.
    pub const fn new(tag: u32) -> Self {
        Self {
            tag,
        }
    }
}

unsafe impl GlobalAlloc for KernelAllocator {
    /// Uses [`ExAllocatePool2`] on Microsoft Windows 10.0.19041 and later, and
    /// [`ExAllocatePoolWithTag`] on older versions of Microsoft Windows to allocate memory.
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let ptr = ExAllocatePool2(
            // POOL_FLAG_NON_PAGED = 0x0000000000000040UI64 = 64
            64,
            layout.size() as u64,
            self.tag,
        );

        // if ptr.is_null() {
        //     panic!("[kernel-alloc] failed to allocate pool.");
        // }

        ptr as _
    }

    /// Uses [`ExFreePool`] to free allocated memory.
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        ExFreePool(ptr as _)
    }
}