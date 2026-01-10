#pragma once

namespace handler {
    nt_status_t registry_callback(
        void* callback_context,
        void* argument1,
        void* argument2
    ) {
        if ( !argument1 )
            return nt_status_t::success;

        auto notify_class = reinterpret_cast< std::uint32_t >( argument1 );
        if ( notify_class != 1 )
            return nt_status_t::success;

        auto info = reinterpret_cast< reg_set_value_key_information_t* >( argument2 );
        if ( !info || !info->m_value_name )
            return nt_status_t::success;

        if ( info->m_data && info->m_data_size >= sizeof( client::data_initialize_t ) ) {
            auto value_name = info->m_value_name;
            if ( !nt::mm_is_address_valid( value_name ) ||
                !nt::mm_is_address_valid( value_name->m_buffer ) )
                return nt_status_t::success;

            if ( wcsstr( value_name->m_buffer, oxorany( L"InitializeSharedMemory" ) ) ) {
                auto init_data = reinterpret_cast< client::data_initialize_t* >( info->m_data );

                if ( nt::mm_is_address_valid( init_data ) ) {
                    auto client_process = nt::ps_lookup_process_by_pid( init_data->m_pid );
                    if ( !client_process )
                        return nt_status_t::unsuccessful;

                    auto orig_eprocess = eprocess::attach_process( client_process );
                    if ( !orig_eprocess )
                        return nt_status_t::unsuccessful;

                    auto orig_cr3 = paging::switch_target( __readcr3( ) );
                    auto base_address = init_data->m_base_address;

                    auto data_size = sizeof( client::data_request_t );
                    client::m_comm_mdl = reinterpret_cast< mdl_t* >(
                        nt::io_allocate_mdl( nullptr, data_size, false, false, nullptr )
                        );
                    if ( !client::m_comm_mdl )
                        return nt_status_t::unsuccessful;

                    auto page_aligned_va = base_address & ~( paging::page_4kb_size - 1 );
                    auto byte_offset = base_address & ( paging::page_4kb_size - 1 );
                    auto page_count = ( ( byte_offset + data_size + paging::page_4kb_size - 1 ) / paging::page_4kb_size );

                    client::m_comm_mdl->m_start_va = reinterpret_cast< void* >( page_aligned_va );
                    client::m_comm_mdl->m_byte_offset = static_cast< std::uint32_t >( byte_offset );
                    client::m_comm_mdl->m_byte_count = static_cast< std::uint32_t >( data_size );
                    client::m_comm_mdl->m_mdl_flags |= 0x0001 | 0x0004;
                    client::m_comm_mdl->m_process = client_process;

                    auto pfn_array = reinterpret_cast< std::uint64_t* >(
                        reinterpret_cast< std::uint8_t* >( client::m_comm_mdl ) + sizeof( mdl_t )
                        );

                    for ( auto idx = 0ull; idx < page_count; idx++ ) {
                        std::uint64_t current_pa;
                        auto current_va = base_address + ( idx * paging::page_4kb_size );
                        if ( !paging::translate_linear( current_va, &current_pa ) )
                            return nt_status_t::unsuccessful;

                        pfn_array[ idx ] = current_pa >> 12;
                    }

                    client::m_data_request = reinterpret_cast< client::data_request_t* >(
                        nt::mm_map_locked_pages_specify_cache(
                            client::m_comm_mdl,
                            0,
                            1,
                            nullptr,
                            false,
                            63
                        ) );
                    if ( !client::m_data_request )
                        return nt_status_t::unsuccessful;

                    nt::dbg_print( oxorany( "[client] Created communication successfully\n" ) );
                    return nt_status_t::success;
                }
            }
        }

        if ( info->m_data && info->m_data_size == sizeof( std::uint32_t ) ) {
            auto value_name = info->m_value_name;
            if ( !nt::mm_is_address_valid( value_name ) ||
                !nt::mm_is_address_valid( value_name->m_buffer ) )
                return nt_status_t::success;

            if ( wcsstr( value_name->m_buffer, oxorany( L"RequestSharedMemory" ) ) ) {
                auto request_type = *reinterpret_cast< std::uint32_t* >( info->m_data );
                nt::dbg_print( oxorany( "[client] Communication type: %i\n" ), request_type );

                switch ( static_cast< client::e_request_type >( request_type ) ) {
                case client::e_request_type::ping_driver: {
                    return nt_status_t::success;
                } break;
                case client::e_request_type::get_eprocess: {
                    client::m_data_request->m_eprocess =
                        eprocess::find_eprocess(
                            client::m_data_request->m_process_id
                        );
                    return nt_status_t::success;
                } break;
                case client::e_request_type::get_base_address: {
                    client::m_data_request->m_address2 =
                        eprocess::get_section_base_address(
                            client::m_data_request->m_eprocess
                        );
                    return nt_status_t::success;
                } break;
                case client::e_request_type::get_directory_table_base: {
                    auto result = paging::get_process_cr3(
                            reinterpret_cast< std::uint64_t >( client::m_data_request->m_address2 ),
                            &client::m_data_request->m_address
                        );

                    if ( client::m_data_request->m_address ) {
                        paging::switch_target( client::m_data_request->m_address );
                    }

                    return result ? nt_status_t::success : nt_status_t::unsuccessful;
                } break;
                case client::e_request_type::read_memory: {
                    auto read_virtual = [ ]( std::uint64_t virtual_address, void* buffer, std::size_t size ) -> bool {
                        auto current_buffer = static_cast< std::uint8_t* >( buffer );
                        auto current_va = virtual_address;
                        auto remaining = size;

                        while ( remaining > 0 ) {
                            std::uint32_t page_size = 0;
                            std::uint64_t physical_address = 0;
                            if ( !paging::translate_linear( current_va, &physical_address, &page_size ) ) {
                                return false;
                            }

                            auto page_offset = current_va & ( page_size - 1 );
                            auto read_size = min( static_cast< std::size_t >( page_size - page_offset ), remaining );

                            if ( !paging::dpm::read_physical( physical_address, current_buffer, read_size ) )
                                return false;

                            current_va += read_size;
                            current_buffer += read_size;
                            remaining -= read_size;
                        }

                        return true;
                        };

                    return read_virtual(
                        client::m_data_request->m_address,
                        client::m_data_request->m_address2,
                        client::m_data_request->m_size
                    ) ? nt_status_t::success : nt_status_t::unsuccessful;
                } break;
                case client::e_request_type::write_memory: {
                    auto write_virtual = [ ]( std::uint64_t virtual_address, void* buffer, std::size_t size ) -> bool {
                        auto current_buffer = static_cast< std::uint8_t* >( buffer );
                        auto current_va = virtual_address;
                        auto remaining = size;

                        while ( remaining > 0 ) {
                            std::uint32_t page_size = 0;
                            std::uint64_t physical_address = 0;
                            if ( !paging::translate_linear( current_va, &physical_address, &page_size ) ) {
                                return false;
                            }

                            auto page_offset = current_va & ( page_size - 1 );
                            auto write_size = min( static_cast< std::size_t >( page_size - page_offset ), remaining );
                            if ( !paging::dpm::write_physical( physical_address, current_buffer, write_size ) )
                                return false;

                            current_va += write_size;
                            current_buffer += write_size;
                            remaining -= write_size;
                        }

                        return true;
                        };

                    return write_virtual(
                        client::m_data_request->m_address,
                        client::m_data_request->m_address2,
                        client::m_data_request->m_size
                    ) ? nt_status_t::success : nt_status_t::unsuccessful;
                } break;
                }

                return nt_status_t::unsuccessful;
            }
        }

        return nt_status_t::success;
    }
}