EXTERN(_postform_config);

SECTIONS
{
    .interned_strings 0 (INFO):
    {
        __InternedDebugStart = .;
        *(.interned_strings.debug)
        __InternedDebugEnd = .;
        __InternedInfoStart = .;
        *(.interned_strings.info)
        __InternedInfoEnd = .;
        __InternedWarningStart = .;
        *(.interned_strings.warning)
        __InternedWarningEnd = .;
        __InternedErrorStart = .;
        *(.interned_strings.error)
        __InternedErrorEnd = .;
        *(.interned_strings.user)
    }

    .postform_config 0 (INFO):
    {
        KEEP(*(.postform_config))
    }

    .postform_version 0 (INFO):
    {
        KEEP(*(.postform_version))
    }
}
