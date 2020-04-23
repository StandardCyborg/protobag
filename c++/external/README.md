We include these external dependencies as submodules because:
 * We need to build them from source for iOS releases.  FMI see 
    [ProtobagCocoa](../../ProtobagCocoa)
 * We want to pin (and statically link)`libprotobag` and `ProtobagCocoa` to
    the same versions of these dependencies.
