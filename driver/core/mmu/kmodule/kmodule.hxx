#pragma once

namespace kmodule {
    std::uint64_t find_unused_space( void* base, std::uint32_t section_size, std::size_t space_size ) {
        auto* data_section = static_cast< std::uint8_t* >( base );
        for ( auto idx = 0; idx <= section_size - space_size; ) {
            bool found_space = true;
            for ( auto j = 0; j < space_size; ++j ) {
                if ( data_section[ idx + j ] != 0x00 ) {
                    found_space = false;
                    idx += j + 1;
                    break;
                }
            }

            if ( found_space ) {
                return reinterpret_cast< std::uint64_t >( &data_section[ idx ] );
            }
        }

        return 0;
    }

    std::uint64_t find_unused_space( std::size_t space_size ) {
        auto ps_loaded_module_list = reinterpret_cast< list_entry_t* >(
            nt::get_export( oxorany( "PsLoadedModuleList" ) )
            );
        if ( !ps_loaded_module_list )
            return 0;

        auto iter_ldr_entry = reinterpret_cast< kldr_data_table_entry_t* >(
            ps_loaded_module_list->m_flink
            );

        std::uint64_t allocation_base = 0;
        kldr_data_table_entry_t* prev_ldr_entry = nullptr;

        while ( reinterpret_cast< list_entry_t* >( iter_ldr_entry ) != ps_loaded_module_list ) {
            auto module_base = reinterpret_cast< std::uint64_t >( iter_ldr_entry->m_dll_base );
            auto dos_header = reinterpret_cast< dos_header_t* >( module_base );
            if ( !dos_header->is_valid( ) )
                return 0;

            auto nt_headers = reinterpret_cast< nt_headers_t* >( module_base + dos_header->m_lfanew );
            if ( !nt_headers->is_valid( ) )
                return 0;

            auto section_header = reinterpret_cast< section_header_t* >(
                reinterpret_cast< std::uintptr_t >( nt_headers ) +
                nt_headers->m_size_of_optional_header + 0x18 );

            for ( auto idx = 0; idx < nt_headers->m_number_of_sections; idx++ ) {
                if ( ( section_header[ idx ].m_characteristics & 0x20000000 ) )
                    continue;

                auto section_base = module_base + section_header[ idx ].m_virtual_address;
                auto section_size = section_header[ idx ].m_virtual_size;

                auto target_address = find_unused_space(
                    reinterpret_cast < void* >( section_base ),
                    section_size,
                    space_size
                );

                if ( !target_address )
                    continue;

                paging::switch_target( paging::m_system_dtb );
                //if ( !paging::spoof_pte_range( unused_space, space_size ) ) {
                //    nt::dbg_print( oxorany( "[kmodule] spoof_pte_range: Could not spoof pte range\n" ) );
                //    return 0;
                //}

                return target_address;
            }

            iter_ldr_entry = reinterpret_cast< kldr_data_table_entry_t* >(
                iter_ldr_entry->m_in_load_order_links.m_flink
                );
        }

        return 0;
    }
}