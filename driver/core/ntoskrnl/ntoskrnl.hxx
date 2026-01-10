#pragma once

namespace nt {
    extern "C" std::uint64_t get_base_address( );
    std::uint64_t m_base_address;

    bool get_section( const char* section_name, std::uint64_t* exec_base, std::uint64_t* exec_size ) {
        auto dos_header{ reinterpret_cast< dos_header_t* > ( m_base_address ) };
        auto nt_headers{ reinterpret_cast< nt_headers_t* > ( m_base_address + dos_header->m_lfanew ) };
        if ( !dos_header->is_valid( )
            || !nt_headers->is_valid( ) )
            return false;

        auto section_header = reinterpret_cast< section_header_t* >(
            reinterpret_cast< std::uintptr_t >( nt_headers ) +
            nt_headers->m_size_of_optional_header + 0x18 );

        for ( int i = 0; i < nt_headers->m_number_of_sections; i++ ) {
            auto current_section_base = reinterpret_cast< std::uint64_t >( dos_header ) + section_header[ i ].m_virtual_address;
            if ( !std::strcmp( section_header[ i ].m_name, section_name ) ) {
                *exec_base = current_section_base;
                *exec_size = section_header[ i ].m_size_of_raw_data;
                break;
            }
        }

        return *exec_base && *exec_size;
    }

    bool next_exec_section( std::uint64_t* exec_base, std::uint64_t* exec_size ) {
        auto dos_header{ reinterpret_cast< dos_header_t* > ( m_base_address ) };
        auto nt_headers{ reinterpret_cast< nt_headers_t* > ( m_base_address + dos_header->m_lfanew ) };
        if ( !dos_header->is_valid( )
            || !nt_headers->is_valid( ) )
            return false;

        auto section_header = reinterpret_cast< section_header_t* >(
            reinterpret_cast< std::uintptr_t >( nt_headers ) +
            nt_headers->m_size_of_optional_header + 0x18 );

        for ( int i = 0; i < nt_headers->m_number_of_sections; i++ ) {
            auto current_section_base = reinterpret_cast< std::uint64_t >( dos_header ) + section_header[ i ].m_virtual_address;
            if ( section_header[ i ].m_characteristics & 0x20000000 ) {
                *exec_base = current_section_base;
                *exec_size = section_header[ i ].m_size_of_raw_data;
                break;
            }
        }

        return *exec_base && *exec_size;
    }

    std::uint64_t get_export( const char* export_name ) {
        auto dos_header{ reinterpret_cast< dos_header_t* > ( m_base_address ) };
        auto nt_headers{ reinterpret_cast< nt_headers_t* > ( m_base_address + dos_header->m_lfanew ) };
        if ( !dos_header->is_valid( )
            || !nt_headers->is_valid( ) )
            return {};

        auto exp_dir{ nt_headers->m_export_table.as_rva< export_directory_t* >( m_base_address ) };
        if ( !exp_dir->m_address_of_functions
            || !exp_dir->m_address_of_names
            || !exp_dir->m_address_of_names_ordinals )
            return {};

        auto name{ reinterpret_cast< std::int32_t* > ( m_base_address + exp_dir->m_address_of_names ) };
        auto func{ reinterpret_cast< std::int32_t* > ( m_base_address + exp_dir->m_address_of_functions ) };
        auto ords{ reinterpret_cast< std::int16_t* > ( m_base_address + exp_dir->m_address_of_names_ordinals ) };

        for ( std::int32_t i{}; i < exp_dir->m_number_of_names; i++ ) {
            auto cur_name{ m_base_address + name[ i ] };
            auto cur_func{ m_base_address + func[ ords[ i ] ] };
            if ( !cur_name
                || !cur_func )
                continue;

            if ( std::strcmp( export_name, reinterpret_cast< char* > ( cur_name ) ) == 0 )
                return cur_func;
        }
        return {};
    }

    std::uintptr_t find_signature( std::uintptr_t base, std::size_t size, const std::uint8_t* signature, const char* mask ) {
        const auto sig_length = std::strlen( mask );

        if ( sig_length == 0 || size < sig_length )
            return 0;

        for ( std::size_t i = 0; i <= size - sig_length; ++i ) {
            bool found = true;

            for ( std::size_t j = 0; j < sig_length; ++j ) {
                if ( mask[ j ] == 'x' &&
                    *reinterpret_cast< const std::uint8_t* >( base + i + j ) != signature[ j ] ) {
                    found = false;
                    break;
                }
            }

            if ( found )
                return base + i;
        }

        return 0;
    }

    std::uintptr_t find_ida_pattern( std::uintptr_t base, std::size_t size, const char* ida_pattern ) {
        std::uint8_t pattern[ 256 ];
        char mask[ 256 ];
        std::size_t pattern_size = 0;

        const char* ptr = ida_pattern;
        while ( *ptr ) {
            if ( *ptr == ' ' ) {
                ptr++;
                continue;
            }

            if ( *ptr == '?' ) {
                mask[ pattern_size ] = '?';
                pattern[ pattern_size++ ] = 0;
                ptr++;

                if ( *ptr == '?' ) ptr++;
            }
            else {
                char byte_str[ 3 ] = { ptr[ 0 ], ptr[ 1 ], 0 };
                pattern[ pattern_size ] = static_cast< std::uint8_t >( std::strtoul( byte_str, nullptr, 16 ) );
                mask[ pattern_size++ ] = 'x';
                ptr += 2;
            }

            if ( *ptr == ' ' ) ptr++;
        }

        mask[ pattern_size ] = 0;

        for ( std::size_t i = 0; i < pattern_size; i++ ) {
            if ( mask[ i ] == '?' ) mask[ i ] = '?';
            else mask[ i ] = 'x';
        }

        return find_signature( base, size, pattern, mask );
    }

    std::uintptr_t scan_ida_pattern( const char* ida_pattern ) {
        std::uint64_t text_base = 0;
        std::uint64_t text_size = 0;

        if ( !next_exec_section( &text_base, &text_size ) )
            return 0;

        return find_ida_pattern( text_base, text_size, ida_pattern );
    }
}