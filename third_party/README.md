# Third-party source dependencies

MINT's `make raylib` target downloads the official raylib 6.2 source archive into
`third_party/raylib/` on first use, then builds its static library locally.
The source is not copied into the MINT runtime and remains a separately
licensed upstream dependency. See the upstream `LICENSE` file after fetching.

The download is pinned to the `6.2` release tag. Review upstream changes before
changing the pinned version.


Note: this release still fetches raylib on the first `make raylib` build; the raylib source archive is not embedded in the MINT 2.6.0 ZIP. The runtime and binding tests do not require a real raylib installation.
