#pragma once

namespace eprocess {
	std::uint64_t get_directory_table_base( eprocess_t* process ) {
		return process->m_pcb.m_directory_table_base;
	}

	const char* get_image_file_name( eprocess_t* eprocess ) {
		auto current_process = nt::ps_get_current_process( );
		if ( !nt::mm_is_address_valid( current_process ) )
			return 0;

		auto image_file_name_offset = nt::get_image_file_name_offset( );
		auto image_file_name_ptr = reinterpret_cast< const char* >(
			reinterpret_cast< std::uintptr_t >( current_process ) + image_file_name_offset
			);

		if ( !nt::mm_is_address_valid( const_cast< char* >( image_file_name_ptr ) ) )
			return 0;

		char image_file_name[ 16 ] = { 0 };
		__try {
			for ( int i = 0; i < 15; i++ ) {
				image_file_name[ i ] = image_file_name_ptr[ i ];
				if ( image_file_name[ i ] == '\0' )
					break;
			}
			image_file_name[ 15 ] = '\0';
		}
		__except ( 1 ) {
			return 0;
		}

		return image_file_name;
	}

	void* get_section_base_address( eprocess_t* eprocess ) {
		auto section_base_address_offset = nt::get_section_base_address_offset( );
		return *reinterpret_cast< void** >(
			reinterpret_cast< std::uintptr_t >( eprocess ) + section_base_address_offset
			);
	}

	eprocess_t* find_eprocess( std::uint32_t target_pid ) {
		auto process_list_head = nt::ps_active_process_head( );
		if ( !process_list_head )
			return nullptr;

		auto linkage_va = reinterpret_cast< std::uintptr_t >( process_list_head ) -
			nt::ps_initial_system_process( );
		if ( !linkage_va )
			return nullptr;

		for ( auto flink = process_list_head->m_flink; flink; flink = flink->m_flink ) {
			if ( !nt::mm_is_address_valid( flink ) )
				break;

			auto curr_eprocess = reinterpret_cast< eprocess_t* >(
				reinterpret_cast< std::uintptr_t >( flink ) - linkage_va
				);
			if ( !curr_eprocess )
				continue;

			auto process_id = nt::ps_get_process_id( curr_eprocess );
			if ( process_id == target_pid )
				return curr_eprocess;
		}

		return nullptr;
	}


	eprocess_t* attach_process( eprocess_t* eprocess ) {
		auto current_thread = reinterpret_cast< ethread_t* >( __readgsqword( 0x188 ) );
		if ( !current_thread )
			return nullptr;

		auto apc_state = &current_thread->m_kthread.m_apc_state;
		auto org_process = apc_state->m_process;

		apc_state->m_process = eprocess;
		__writecr3( eprocess->m_pcb.m_directory_table_base );
		return org_process;
	}
}