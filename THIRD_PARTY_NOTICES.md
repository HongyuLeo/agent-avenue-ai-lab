# Third-party notices

The source repository has no required third-party runtime library beyond the C++ standard library and operating-system APIs.

The prebuilt Windows release is compiled with Zig 0.13.0 and may incorporate static runtime components from Zig, MinGW-w64, libc++, libc++abi, and libunwind. Those components retain their respective licenses. See the complete `THIRD_PARTY_NOTICES.txt` shipped inside the Windows release archive.

CUDA is optional and is loaded dynamically from an existing NVIDIA installation. No NVIDIA runtime is redistributed here.
