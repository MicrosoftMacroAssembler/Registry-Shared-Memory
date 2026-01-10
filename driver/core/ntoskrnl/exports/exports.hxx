#pragma once

namespace nt {
    kthread_t* ke_get_current_thread( ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = nt::get_export( oxorany( "KeGetCurrentThread" ) );
            if ( !fn_address ) return {};
        }

        using function_t = kthread_t * ( );
        return reinterpret_cast< function_t* >( fn_address )( );
    }

    std::uintptr_t ps_initial_system_process( ) {
        static std::uint64_t fn_address;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "PsInitialSystemProcess" ) );
            if ( !fn_address ) return {};
        }

        return *reinterpret_cast< std::uintptr_t* >( fn_address );
    }

    void* map_io_space_ex(
        physical_address_t physical_address,
        std::size_t number_of_bytes,
        std::uint32_t protect
    ) {
        static std::uint64_t fn_address;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmMapIoSpaceEx" ) );
            if ( !fn_address ) return { };
        }

        using function_t = void* (
            physical_address_t,
            std::size_t,
            std::uint32_t
            );

        return reinterpret_cast< function_t* >( fn_address ) (
            physical_address,
            number_of_bytes,
            protect );
    }

    void mm_unmap_io_space(
        void* base_address,
        std::size_t number_of_bytes
    ) {
        static std::uint64_t fn_address;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmUnmapIoSpace" ) );
            if ( !fn_address ) return;
        }

        using function_t = void(
            void* base_address,
            std::size_t number_of_bytes
            );

        reinterpret_cast< function_t* >( fn_address ) (
            base_address,
            number_of_bytes );
    }

    nt_status_t mm_copy_memory(
        void* target_address,
        mm_copy_address_t source_address,
        std::size_t number_of_bytes,
        std::uint32_t flags,
        std::size_t* number_of_bytes_transferred
    ) {
        static std::uint64_t fn_address;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmCopyMemory" ) );
            if ( !fn_address ) return { };
        }

        using function_t = nt_status_t( void*, mm_copy_address_t, std::size_t, std::uint32_t, std::size_t* );
        return reinterpret_cast< function_t* >( fn_address )(
            target_address,
            source_address,
            number_of_bytes,
            flags,
            number_of_bytes_transferred
            );
    }

    ethread_t* ps_lookup_thread_by_tid( std::uint32_t thread_id ) {
        static void* ps_lookup_thread_by_tid = nullptr;
        if ( !ps_lookup_thread_by_tid ) {
            ps_lookup_thread_by_tid = reinterpret_cast< void* >(
                get_export(
                    oxorany( "PsLookupThreadByThreadId" )
                )
                );

            if ( !ps_lookup_thread_by_tid )
                return nullptr;
        }

        ethread_t* thread = nullptr;
        using function_t = nt_status_t( * )( HANDLE thread_id, ethread_t** thread );
        auto status = reinterpret_cast< function_t >( ps_lookup_thread_by_tid )(
            reinterpret_cast< HANDLE >( thread_id ),
            &thread
            );

        if ( !status ) {
            return thread;
        }

        return nullptr;
    }

    nt_status_t ps_set_create_thread_notify_routine( p_create_thread_notify_routine callback_routine ) {
        static std::uint8_t* fn_address = nullptr;
        if ( !fn_address ) {
            fn_address = reinterpret_cast< std::uint8_t* >(
                get_export( oxorany( "PsSetCreateThreadNotifyRoutine" ) )
                );
            if ( !fn_address ) return {};
        }

        using function_t = nt_status_t( __stdcall* )( p_create_thread_notify_routine );
        auto fn = reinterpret_cast< function_t >( fn_address );

        return fn( callback_routine );
    }

    mdl_t* io_allocate_mdl(
        void* virtual_address,
        std::size_t length,
        bool secondary_buffer,
        bool charge_quota,
        iop_irp_t* irp
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "IoAllocateMdl" ) );
            if ( !fn_address ) return {};
        }

        using function_t = mdl_t * (
            void* virtual_address,
            std::size_t length,
            bool secondary_buffer,
            bool charge_quota,
            iop_irp_t* irp
            );

        return reinterpret_cast< function_t* >( fn_address ) (
            virtual_address,
            length,
            secondary_buffer,
            charge_quota,
            irp );
    }

    void mm_build_mdl_for_non_paged_pool(
        mdl_t* mdl
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmBuildMdlForNonPagedPool" ) );
            if ( !fn_address ) return;
        }

        using function_t = void(
            mdl_t* mdl
            );

        reinterpret_cast< function_t* >( fn_address ) ( mdl );
    }

    void* mm_map_locked_pages(
        mdl_t* mdl,
        std::uint8_t access_mode
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmMapLockedPages" ) );
            if ( !fn_address ) return {};
        }

        using function_t = void* (
            mdl_t* mdl,
            std::uint8_t access_mode
            );

        return reinterpret_cast< function_t* >( fn_address ) (
            mdl,
            access_mode );
    }

    void mm_probe_and_lock_pages(
        mdl_t* mdl,
        std::uint8_t access_mode,
        std::uint32_t operation
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmProbeAndLockPages" ) );
            if ( !fn_address ) return;
        }

        using function_t = void(
            mdl_t* mdl,
            std::uint8_t access_mode,
            std::uint32_t operation
            );

        reinterpret_cast< function_t* >( fn_address ) (
            mdl,
            access_mode,
            operation );
    }

    void* mm_map_locked_pages_specify_cache( mdl_t* mdl,
        std::uint8_t access_mode,
        std::uint32_t cache_type,
        void* base_address,
        bool bug_check_on_failure,
        std::uint32_t priority
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmMapLockedPagesSpecifyCache" ) );
            if ( !fn_address ) return {};
        }

        using function_t = void* (
            mdl_t* mdl,
            std::uint8_t access_mode,
            std::uint32_t cache_type,
            void* base_address,
            bool bug_check_on_failure,
            std::uint32_t priority
            );

        return reinterpret_cast< function_t* >( fn_address ) (
            mdl,
            access_mode,
            cache_type,
            base_address,
            bug_check_on_failure,
            priority );
    }

    nt_status_t mm_protect_mdl_system_address( mdl_t* mdl, std::uint32_t new_protect ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmProtectMdlSystemAddress" ) );
            if ( !fn_address ) return {};
        }
        using function_t = nt_status_t(
            mdl_t* mdl,
            std::uint32_t new_protect
        );

        return reinterpret_cast< function_t* >( fn_address ) (
            mdl,
            new_protect );
    }

    void mm_unmap_locked_pages( void* base_address, mdl_t* mdl ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmUnmapLockedPages" ) );
            if ( !fn_address ) return;
        }
        using function_t = void(
            void* base_address,
            mdl_t* mdl
            );
        reinterpret_cast< function_t* >( fn_address ) (
            base_address,
            mdl );
    }

    void mm_unlock_pages( mdl_t* mdl ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmUnlockPages" ) );
            if ( !fn_address ) return;
        }
        using function_t = void( mdl_t* mdl );
        reinterpret_cast< function_t* >( fn_address )( mdl );
    }

    void io_free_mdl( mdl_t* mdl ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "IoFreeMdl" ) );
            if ( !fn_address ) return;
        }
        using function_t = void( mdl_t* mdl );
        reinterpret_cast< function_t* >( fn_address )( mdl );
    }

    template<class... args_t>
    std::int8_t dbg_print( const char* format, args_t... va_args ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "DbgPrintEx" ) );
            if ( !fn_address ) return 0;
        }

        using function_t = std::int32_t( std::uint32_t flag, std::uint32_t level,
            const char* format, args_t... va_args );
        return reinterpret_cast< function_t* >( fn_address )( 0, 0, format, va_args... ) ==
            nt_status_t::success;
    }

    std::uint64_t get_thread_start_address( ethread_t* thread ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = scan_ida_pattern( oxorany( "F7 41 ? ? ? ? ? 4C 8B C9" ) );
            if ( !fn_address ) return 0;
        }

        using function_t = std::uint64_t( ethread_t*, int );
        return reinterpret_cast< function_t* >( fn_address )( thread, 0 );
    }

    void rtl_init_unicode_string( unicode_string_t* destination_string, const wchar_t* source_string ) {
        static std::uint64_t fn_address;
        if ( !fn_address )
            fn_address = get_export( oxorany( "RtlInitUnicodeString" ) );

        using fn_t = void( * )( unicode_string_t*, const wchar_t* );
        reinterpret_cast< fn_t >( fn_address )( destination_string, source_string );
    }

    nt_status_t cm_register_callback_ex(
        ex_callback_function_t callback_function,
        unicode_string_t altitude,
        ularge_integer_t* cookie
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "CmRegisterCallbackEx" ) );
            if ( !fn_address ) return {};
        }

        using function_t = nt_status_t(
            ex_callback_function_t callback_function,
            unicode_string_t* altitude,
            void* driver,
            void* context,
            ularge_integer_t* cookie,
            std::uint64_t reserved
        );

        return reinterpret_cast< function_t* >( fn_address )(
            callback_function,
            &altitude,
            reinterpret_cast< void* >( 1 ),
            nullptr,
            cookie,
            0
            );
    }

    eprocess_t* ps_get_current_process( ) {
        static auto ps_get_current_process = 0ull;
        if ( !ps_get_current_process ) {
            ps_get_current_process = nt::get_export( oxorany( "PsGetCurrentProcess" ) );
            if ( !ps_get_current_process ) return {};
        }

        using function_t = eprocess_t * ( );
        return reinterpret_cast< function_t* >( ps_get_current_process )( );
    }

    bool mm_is_address_valid( void* virtual_address ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmIsAddressValid" ) );
            if ( !fn_address ) return {};
        }

        using function_t = bool( void* virtual_address );
        return reinterpret_cast< function_t* >( fn_address )( virtual_address );
    }

    std::uint32_t get_image_file_name_offset( ) {
        static std::uint32_t image_file_name_offset = 0;
        if ( !image_file_name_offset ) {
            auto ps_get_process_image_file_name = reinterpret_cast< std::uint8_t* >(
                nt::get_export( oxorany( "PsGetProcessImageFileName" ) )
                );

            if ( !ps_get_process_image_file_name ) {
                return 0;
            }

            while ( !( ps_get_process_image_file_name[ 0 ] == 0x48 &&
                ps_get_process_image_file_name[ 1 ] == 0x8D &&
                ps_get_process_image_file_name[ 2 ] == 0x81 ) ) {
                ps_get_process_image_file_name++;
            }

            image_file_name_offset = *reinterpret_cast< std::uint32_t* >( ps_get_process_image_file_name + 3 );
        }

        return image_file_name_offset;
    }

    eprocess_t* ps_lookup_process_by_pid( std::uint32_t process_id ) {
        static void* fn_ps_lookup_process_by_pid = nullptr;
        if ( !fn_ps_lookup_process_by_pid ) {
            fn_ps_lookup_process_by_pid = reinterpret_cast< void* >(
                nt::get_export(
                    oxorany( "PsLookupProcessByProcessId" )
                )
                );

            if ( !fn_ps_lookup_process_by_pid )
                return nullptr;
        }

        eprocess_t* process = nullptr;
        using function_t = nt_status_t( * )( HANDLE process_id, eprocess_t** process );
        auto status = reinterpret_cast< function_t >( fn_ps_lookup_process_by_pid )(
            reinterpret_cast< HANDLE >( process_id ),
            &process
            );

        if ( !status ) {
            return process;
        }

        return nullptr;
    }

    nt_status_t ob_reference_object_by_handle(
        void* handle,
        std::uint32_t desired_access,
        void* object_type,
        std::uint8_t access_mode,
        void** object,
        void* handle_information
    ) {
        static auto function_address = 0ull;
        if ( !function_address ) {
            function_address = nt::get_export( oxorany( "ObReferenceObjectByHandle" ) );
            if ( !function_address ) return nt_status_t::unsuccessful;
        }

        using function_t = nt_status_t( * )(
            void*,
            std::uint32_t,
            void*,
            std::uint8_t,
            void**,
            void*
            );

        return reinterpret_cast< function_t >( function_address )(
            handle,
            desired_access,
            object_type,
            access_mode,
            object,
            handle_information
            );
    }

    nt_status_t ob_register_callbacks(
        ob_callback_registration_t* callback_registration,
        void** registration_handle
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ObRegisterCallbacks" ) );
            if ( !fn_address ) return nt_status_t::unsuccessful;
        }

        using function_t = nt_status_t( * )(
            ob_callback_registration_t* callback_registration,
            void** registration_handle
            );

        return reinterpret_cast< function_t >( fn_address )(
            callback_registration,
            registration_handle
            );
    }

    void ob_unregister_callbacks(
        void* registration_handle
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ObUnRegisterCallbacks" ) );
            if ( !fn_address ) return;
        }

        using function_t = void( * )(
            void* registration_handle
            );

        reinterpret_cast< function_t >( fn_address )( registration_handle );
    }

    void** ps_process_type( ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "PsProcessType" ) );
            if ( !fn_address ) return nullptr;
        }

        return reinterpret_cast< void** >( fn_address );
    }

    void* ps_thread_type( ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "PsThreadType" ) );
            if ( !fn_address ) return nullptr;
        }

        return *reinterpret_cast< void** >( fn_address );
    }

    void* mm_get_virtual_for_physical( std::uintptr_t phys_addr ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "MmGetVirtualForPhysical" ) );
            if ( !fn_address ) return nullptr;
        }

        using function_t = void* ( * )( std::uintptr_t physical_address );
        return reinterpret_cast< function_t >( fn_address )( phys_addr );
    }

    std::uint32_t ke_get_current_processor_number( ) {
        static auto fn_ke_get_current_processor_number = 0ull;
        if ( !fn_ke_get_current_processor_number ) {
            fn_ke_get_current_processor_number = nt::get_export( oxorany( "KeGetCurrentProcessorNumberEx" ) );
            if ( !fn_ke_get_current_processor_number ) return {};
        }

        using function_t = std::uint32_t( __int64 );
        return reinterpret_cast< function_t* >( fn_ke_get_current_processor_number )( 0 );
    }

    void* ex_allocate_pool_with_tag( std::uint32_t pool_type, std::size_t number_of_bytes, std::uint32_t tag ) {
        static std::uint64_t fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ExAllocatePoolWithTag" ) );
            if ( !fn_address ) return { };
        }

        using function_t = void* ( * )( std::uint32_t, std::size_t, std::uint32_t );
        return reinterpret_cast< function_t >( fn_address )( pool_type, number_of_bytes, tag );
    }

    void ex_free_pool_with_tag( void* p, std::uint32_t tag ) {
        static std::uint64_t fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ExFreePoolWithTag" ) );
            if ( !fn_address ) return;
        }

        using function_t = void( * )( void*, std::uint32_t );
        reinterpret_cast< function_t >( fn_address )( p, tag );
    }

    nt_status_t po_register_power_setting_callback(
        void* device_object,
        const guid_t* setting_guid,
        power_setting_callback_t callback,
        void* context,
        void** handle
    ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "PoRegisterPowerSettingCallback" ) );
            if ( !fn_address ) return nt_status_t::unsuccessful;
        }

        using function_t = nt_status_t( * )(
            void* device_object,
            const guid_t* setting_guid,
            power_setting_callback_t callback,
            void* context,
            void** handle
            );

        return reinterpret_cast< function_t >( fn_address )(
            device_object,
            setting_guid,
            callback,
            context,
            handle
            );
    }

    nt_status_t ke_wait_for_single_object(
        void* object,
        long wait_reason,
        long wait_mode,
        bool alertable,
        int wait_duration
    ) {
        static auto function_address = 0ull;
        if ( !function_address ) {
            function_address = nt::get_export( oxorany( "KeWaitForSingleObject" ) );
            if ( !function_address ) return nt_status_t::unsuccessful;
        }

        using function_t = nt_status_t( * )(
            void*,
            long,
            long,
            bool,
            ularge_integer_t*
            );

        ularge_integer_t timeout{ };
        timeout.m_quad_part = wait_duration * -10000i64;
        return reinterpret_cast< function_t >( function_address )(
            object,
            wait_reason,
            wait_mode,
            alertable,
            &timeout
            );
    }

    long ke_release_semaphore(
        ksemaphore_t* semaphore,
        long increment,
        long adjustment,
        bool wait
    ) {
        static auto function_address = 0ull;
        if ( !function_address ) {
            function_address = nt::get_export( oxorany( "KeReleaseSemaphore" ) );
            if ( !function_address ) return {};
        }

        using function_t = long ( * )(
            ksemaphore_t*,
            long,
            long,
            bool
            );

        return reinterpret_cast< function_t >( function_address )(
            semaphore,
            increment,
            adjustment,
            wait
            );
    }

    nt_status_t zw_create_file(
        void* file_handle,
        std::uint32_t desired_access,
        object_attributes_t* object_attributes,
        io_status_block_t* io_status,
        ularge_integer_t* allocation_size,
        std::uint32_t file_attributes,
        std::uint32_t share_access,
        std::uint32_t create_disposition,
        std::uint32_t create_options,
        void* ea_buffer,
        std::uint32_t ea_length
    ) {
        static std::uint64_t fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ZwCreateFile" ) );
            if ( !fn_address ) return { };
        }

        using function_t = nt_status_t( * )(
            void*, std::uint32_t, object_attributes_t*, io_status_block_t*, ularge_integer_t*,
            std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, void*, std::uint32_t );
        return reinterpret_cast< function_t >( fn_address )(
            file_handle, desired_access, object_attributes, io_status, allocation_size,
            file_attributes, share_access, create_disposition, create_options, ea_buffer, ea_length );
    }

    nt_status_t zw_query_information_file(
        void* file_handle,
        io_status_block_t* io_status,
        void* file_information,
        std::uint32_t length,
        std::uint32_t file_information_class
    ) {
        static std::uint64_t fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ZwQueryInformationFile" ) );
            if ( !fn_address ) return { };
        }

        using function_t = nt_status_t( * )( void*, io_status_block_t*, void*, std::uint32_t, std::uint32_t );
        return reinterpret_cast< function_t >( fn_address )( file_handle, io_status, file_information, length, file_information_class );
    }

    nt_status_t zw_read_file(
        void* file_handle,
        void* event_handle,
        void* apc_routine,
        void* apc_context,
        io_status_block_t* io_status,
        void* buffer,
        std::uint32_t length,
        ularge_integer_t* byte_offset,
        std::uint32_t* key
    ) {
        static std::uint64_t fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ZwReadFile" ) );
            if ( !fn_address ) return { };
        }

        using function_t = nt_status_t( * )(
            void*, void*, void*, void*, io_status_block_t*, void*, std::uint32_t, ularge_integer_t*, std::uint32_t* );
        return reinterpret_cast< function_t >( fn_address )(
            file_handle, event_handle, apc_routine, apc_context, io_status, buffer, length, byte_offset, key );
    }

    nt_status_t zw_close( void* handle ) {
        static std::uint64_t fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "ZwClose" ) );
            if ( !fn_address ) return { };
        }

        using function_t = nt_status_t( * )( void* );
        return reinterpret_cast< function_t >( fn_address )( handle );
    }

    list_entry_t* ps_active_process_head( ) {
        static list_entry_t* ps_active_process_head = nullptr;
        if ( !ps_active_process_head ) {
            static std::uint8_t* fn_address;
            if ( !fn_address ) {
                fn_address = reinterpret_cast< std::uint8_t* >(
                    get_export( oxorany( "KeCapturePersistentThreadState" ) )
                    );
                if ( !fn_address ) return {};
            }

            while ( fn_address[ 0x0 ] != 0x20
                || fn_address[ 0x1 ] != 0x48
                || fn_address[ 0x2 ] != 0x8d )
                fn_address++;

            ps_active_process_head = *reinterpret_cast< list_entry_t** >
                ( &fn_address[ 0x8 ] + *reinterpret_cast< std::int32_t* >( &fn_address[ 0x4 ] ) );
        }

        return ps_active_process_head;
    }

    std::uint32_t ps_get_process_id( eprocess_t* process ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = get_export( oxorany( "PsGetProcessId" ) );
            if ( !fn_address ) return {};
        }

        using function_t = std::uint32_t( eprocess_t* process );
        return reinterpret_cast< function_t* >( fn_address )( process );
    }

    std::uint32_t get_section_base_address_offset( ) {
        static std::uint32_t section_base_address_offset = 0;
        if ( !section_base_address_offset ) {
            auto ps_get_process_section_base_address = reinterpret_cast< std::uint8_t* >(
                nt::get_export( oxorany( "PsGetProcessSectionBaseAddress" ) )
                );

            if ( !ps_get_process_section_base_address )
                return { };

            while ( !( ps_get_process_section_base_address[ 0 ] == 0x48 ||
                ps_get_process_section_base_address[ 1 ] == 0x8B ||
                ps_get_process_section_base_address[ 2 ] == 0x81 ) )
                ps_get_process_section_base_address++;

            section_base_address_offset = *reinterpret_cast< std::uint32_t* >( ps_get_process_section_base_address + 3 );
        }

        return section_base_address_offset;
    }

    physical_memory_range_t* mm_get_physical_memory_ranges( ) {
        static auto fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = nt::get_export( oxorany( "MmGetPhysicalMemoryRanges" ) );
            if ( !fn_address ) return nullptr;
        }

        using function_t = physical_memory_range_t * ( void );
        return reinterpret_cast< function_t* >( fn_address )( );
    }

    mmpfn_t* get_mm_pfn_database( ) {
        static std::uint8_t* fn_address = 0ull;
        if ( !fn_address ) {
            fn_address = reinterpret_cast< std::uint8_t* >(
                nt::get_export( oxorany( "KeCapturePersistentThreadState" ) ) );
            if ( !fn_address ) return { };
        }

        while ( fn_address[ 0x0 ] != 0x48
            || fn_address[ 0x1 ] != 0x8B
            || fn_address[ 0x2 ] != 0x05 )
            fn_address++;

        return *reinterpret_cast< mmpfn_t** >(
            &fn_address[ 0x7 ] + *reinterpret_cast< std::int32_t* >( &fn_address[ 0x3 ] ) );
    }
}