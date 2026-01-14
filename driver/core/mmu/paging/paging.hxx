#pragma once

namespace paging {
	std::uint64_t switch_target( std::uint64_t new_dtb ) {
		auto original_dtb = m_target_dtb;
		m_target_dtb = new_dtb;
		return original_dtb;
	}

	bool hyperspace_entries( pt_entries_t& entries, std::uint64_t addr ) {
		virt_addr_t va{ addr };
		if ( !dpm::read_physical( m_target_dtb + ( va.pml4e_index * sizeof( pml4e ) ),
			&entries.m_pml4e, sizeof( pml4e ) ) )
			return false;

		if ( !entries.m_pml4e.hard.present )
			return false;

		if ( !dpm::read_physical( ( entries.m_pml4e.hard.pfn << page_shift ) + ( va.pdpte_index * sizeof( pdpte ) ),
			&entries.m_pdpte, sizeof( pdpte ) ) )
			return false;

		if ( !entries.m_pdpte.hard.present )
			return false;

		if ( !dpm::read_physical( ( entries.m_pdpte.hard.pfn << page_shift ) + ( va.pde_index * sizeof( pde ) ),
			&entries.m_pde, sizeof( pde ) ) )
			return false;

		if ( !entries.m_pde.hard.present )
			return false;

		if ( entries.m_pde.hard.page_size ) {
			entries.m_pte.value = entries.m_pde.value;
			return true;
		}

		if ( !dpm::read_physical( ( entries.m_pde.hard.pfn << page_shift ) + ( va.pte_index * sizeof( pte ) ),
			&entries.m_pte, sizeof( pte ) ) )
			return false;

		if ( !entries.m_pte.hard.present )
			return false;

		return true;
	}

	bool translate_linear( std::uint64_t va, std::uint64_t* pa = nullptr, std::uint32_t* page_size = nullptr ) {
		pt_entries_t pt_entries;
		if ( !hyperspace_entries( pt_entries, va ) )
			return false;

		if ( !pt_entries.m_pml4e.hard.present )
			return false;

		if ( !pt_entries.m_pdpte.hard.present )
			return false;

		if ( pt_entries.m_pdpte.hard.page_size ) {
			if ( page_size ) *page_size = page_1gb_size;
			if ( pa ) *pa = ( pt_entries.m_pdpte.hard.pfn << page_shift ) + ( va & page_1gb_mask );
			return true;
		}

		if ( !pt_entries.m_pde.hard.present )
			return false;

		if ( pt_entries.m_pde.hard.page_size ) {
			if ( page_size ) *page_size = page_2mb_size;
			if ( pa ) *pa = ( pt_entries.m_pde.hard.pfn << page_shift ) + ( va & page_2mb_mask );
			return true;
		}

		if ( !pt_entries.m_pte.hard.present )
			return false;

		if ( page_size ) *page_size = page_4kb_size;
		if ( pa ) *pa = ( pt_entries.m_pte.hard.pfn << page_shift ) + ( va & page_4kb_mask );
		return true;
	}

	bool spoof_pte_range( std::uint64_t address, std::size_t size, bool execute_disable = false ) {
		const auto page_mask = paging::page_4kb_size - 1;
		const auto aligned_size = ( size + page_mask ) & ~page_mask;

		const auto page_count = aligned_size >> paging::page_shift;
		for ( auto idx = 0; idx < page_count; ++idx ) {
			const auto current_va = address + ( idx << paging::page_shift );
			virt_addr_t va{ current_va };

			pml4e pml4_entry{ };
			if ( !dpm::read_physical( m_target_dtb + ( va.pml4e_index * sizeof( pml4e ) ),
				&pml4_entry, sizeof( pml4e ) ) )
				continue;

			if ( !pml4_entry.hard.present )
				return false;

			pdpte pdpt_entry{ };
			if ( !dpm::read_physical( ( pml4_entry.hard.pfn << page_shift ) + ( va.pdpte_index * sizeof( pdpte ) ),
				&pdpt_entry, sizeof( pdpte ) ) )
				continue;

			if ( !pdpt_entry.hard.present )
				return false;

			if ( pdpt_entry.hard.page_size ) {
				pdpt_entry.hard.user_supervisor = 1;
				pdpt_entry.hard.no_execute = execute_disable ? 1 : 0;

				auto pdpt_phys = ( pml4_entry.hard.pfn << 12 ) + ( va.pdpte_index * sizeof( pdpte ) );
				if ( !dpm::write_physical( pdpt_phys, &pdpt_entry, sizeof( pdpte ) ) ) {
					return false;
				}

				__invlpg( reinterpret_cast< void* >( current_va ) );
				continue;
			}

			pde pd_entry{ };
			if ( !dpm::read_physical( ( pdpt_entry.hard.pfn << page_shift ) + ( va.pde_index * sizeof( pde ) ),
				&pd_entry, sizeof( pde ) ) )
				return false;

			if ( !pd_entry.hard.present )
				return false;

			if ( pd_entry.hard.page_size ) {
				pdpt_entry.hard.user_supervisor = 1;
				pd_entry.hard.no_execute = execute_disable ? 1 : 0;

				auto pd_phys = ( pdpt_entry.hard.pfn << paging::page_shift ) + ( va.pde_index * sizeof( pde ) );
				if ( !dpm::write_physical( pd_phys, &pd_entry, sizeof( pde ) ) ) {
					return false;
				}

				__invlpg( reinterpret_cast< void* >( current_va ) );
				continue;
			}

			pte pt_entry{ };
			if ( !dpm::read_physical( ( pd_entry.hard.pfn << page_shift ) + ( va.pte_index * sizeof( pte ) ),
				&pt_entry, sizeof( pte ) ) )
				return false;

			pt_entry.hard.user_supervisor = 1;
			pt_entry.hard.no_execute = execute_disable ? 1 : 0;

			auto pt_phys = ( pd_entry.hard.pfn << paging::page_shift ) + ( va.pte_index * sizeof( pte ) );
			if ( !dpm::write_physical( pt_phys, &pt_entry, sizeof( pte ) ) ) {
				return false;
			}

			__invlpg( reinterpret_cast< void* >( current_va ) );
		}

		return true;
	}

	mmpfn_t* get_pfn_entry( std::uint64_t pfn ) {
		auto mm_pfn_database = nt::get_mm_pfn_database( );
		if ( !mm_pfn_database ) {
			return nullptr;
		}

		return &mm_pfn_database[ pfn ];
	}

	bool get_process_cr3( std::uint64_t base_address, std::uint64_t* process_cr3 ) {
		virt_addr_t va{ base_address };
		if ( !base_address )
			return false;

		const auto ranges = nt::mm_get_physical_memory_ranges( );
		for ( auto i = 0; ; i++ ) {
			auto memory_range = ranges[ i ];
			if ( !memory_range.m_base_page.m_quad_part ||
				!memory_range.m_page_count.m_quad_part )
				break;

			auto current_pa = memory_range.m_base_page.m_quad_part;
			for ( auto current_page = 0;
				current_page < memory_range.m_page_count.m_quad_part;
				current_page += page_4kb_size, current_pa += page_4kb_size ) {
				cr3 current_dtb{ .dirbase = current_pa >> page_shift };
				if ( !current_dtb.flags )
					continue;

				auto pfn_entry = get_pfn_entry( current_dtb.dirbase );
				if ( !pfn_entry ||
					pfn_entry->m_u4.m_pte_frame != current_dtb.dirbase )
					continue;

				pml4e pml4_entry{};
				if ( !dpm::read_physical( current_pa + ( va.pml4e_index * sizeof( pml4e ) ),
					&pml4_entry, sizeof( pml4e ) ) )
					continue;

				if ( !pml4_entry.hard.present )
					continue;

				pdpte pdpt_entry{};
				if ( !dpm::read_physical( ( pml4_entry.hard.pfn << page_shift ) + ( va.pdpte_index * sizeof( pdpte ) ),
					&pdpt_entry, sizeof( pdpte ) ) )
					continue;

				if ( !pdpt_entry.hard.present )
					continue;

				pde pd_entry{};
				if ( !dpm::read_physical( ( pdpt_entry.hard.pfn << page_shift ) + ( va.pde_index * sizeof( pde ) ),
					&pd_entry, sizeof( pde ) ) )
					continue;

				if ( !pd_entry.hard.present )
					continue;

				pte pt_entry{};
				if ( !dpm::read_physical( ( pd_entry.hard.pfn << page_shift ) + ( va.pte_index * sizeof( pte ) ),
					&pt_entry, sizeof( pte ) ) )
					continue;

				if ( !pt_entry.hard.present )
					continue;

				if ( process_cr3 )
					*process_cr3 = current_dtb.flags;

				nt::dbg_print( oxorany( "[paging] get_process_cr3: Found Process DTB (0x%llx)\n", *process_cr3 );
				return true;
			}
		}
		
		nt::dbg_print( oxorany( "[paging] get_process_cr3: Could not find DTB\n" );
		return false;
	}
}
