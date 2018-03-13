		VIRTUAL_STAT( projectile_block_percent,
			block_percent,
			maximum_block_percent,
			additional_block_chance_against_projectiles_percent,
			keystone_acrobatics_block_chance_pluspercent_final,
			block_chance_pluspercent )
		{
			const float final_scale = Scale( 100 + stats.GetStat( block_chance_pluspercent ) ) *
				Scale( 100 + stats.GetStat( keystone_acrobatics_block_chance_pluspercent_final ) ); //currently two things, and one is multiplicative, so scale just has both multiply with each other.

			return std::min( stats.GetStat( block_percent ) + Round( stats.GetStat( additional_block_chance_against_projectiles_percent ) * final_scale ), stats.GetStat( maximum_block_percent ) );
		}

		VIRTUAL_STAT( spell_block_percent,
			base_spell_block_percent,
			block_percent,
			blocking_blocks_spells_percent,
			maximum_block_percent,
			shield_spell_block_percent, off_hand_weapon_type,
			block_percent_to_apply_to_spells_while_on_low_life, on_low_life,
			keystone_acrobatics_block_chance_pluspercent_final,
			block_chance_pluspercent,
			spell_block_while_dual_wielding_percent, is_dual_wielding,
			spell_block_with_staff_percent, main_hand_weapon_type,
			local_no_block_chance )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool has_shield = off_hand_weapon_index == Items::Shield;
			const bool has_staff = main_hand_weapon_index == Items::Staff;

			const float final_scale = Scale( 100 + stats.GetStat( block_chance_pluspercent ) ) *
				Scale( 100 + stats.GetStat( keystone_acrobatics_block_chance_pluspercent_final ) ); //currently two things, and one is multiplicative, so scale just has both multiply with each other.

			return std::min( Round( stats.GetStat( base_spell_block_percent ) * final_scale +
				( ( has_shield  && !stats.GetStat( local_no_block_chance ) ) ? stats.GetStat( shield_spell_block_percent ) : 0 ) * final_scale +
				stats.GetStat( block_percent ) * Scale( stats.GetStat( blocking_blocks_spells_percent ) + ( stats.GetStat( on_low_life ) ? stats.GetStat( block_percent_to_apply_to_spells_while_on_low_life ) : 0 ) ) +
				( has_staff ? stats.GetStat( spell_block_with_staff_percent ) : 0 ) * final_scale +
				( stats.GetStat( is_dual_wielding ) ? stats.GetStat( spell_block_while_dual_wielding_percent ) : 0 ) * final_scale ),
				stats.GetStat( maximum_block_percent ) );
		}

		VIRTUAL_STAT( spell_projectile_block_percent,
			projectile_block_percent,
			blocking_blocks_spells_percent,
			maximum_block_percent,
			block_percent_to_apply_to_spells_while_on_low_life, on_low_life )
		{
			return std::min( Round( stats.GetStat( projectile_block_percent ) *
				Scale( stats.GetStat( blocking_blocks_spells_percent ) + ( stats.GetStat( on_low_life ) ? stats.GetStat( block_percent_to_apply_to_spells_while_on_low_life ) : 0 ) ) ),
				stats.GetStat( maximum_block_percent ) );
		}

		VIRTUAL_STAT( attack_block_percent,
			block_percent,
			maximum_block_percent,
			cannot_block_attacks )
		{
			if ( stats.GetStat( cannot_block_attacks ) )
				return 0;
			return std::min( stats.GetStat( block_percent ), stats.GetStat( maximum_block_percent ) );
		}

		VIRTUAL_STAT( projectile_attack_block_percent,
			projectile_block_percent,
			maximum_block_percent,
			cannot_block_attacks )
		{
			if ( stats.GetStat( cannot_block_attacks ) )
				return 0;
			return std::min( stats.GetStat( projectile_block_percent ), stats.GetStat( maximum_block_percent ) );
		}
		
		VIRTUAL_STAT( is_dual_wielding,
			main_hand_weapon_type,
			off_hand_weapon_type )
		{
			const signed int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const signed int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			return !!( Items::IsWeapon[size_t( main_hand_weapon_index )] && Items::IsWeapon[size_t( off_hand_weapon_index )] && off_hand_weapon_index != Items::Unarmed );
		}

		VIRTUAL_STAT( maximum_life,
			intermediary_maximum_life,
			keystone_chaos_inoculation,
			additional_player_bonus_maximum_life_pluspercent_final )
		{
			if ( stats.GetStat( keystone_chaos_inoculation ) )
				return 1;

			const float final_scale =
				Scale( 100 + stats.GetStat( additional_player_bonus_maximum_life_pluspercent_final ) );

			return std::max( Round( stats.GetStat( intermediary_maximum_life ) * final_scale ), 1 );
		}

		VIRTUAL_STAT( total_base_maximum_life
			, base_maximum_life
			, life_per_level, level
			, maximum_life_per_10_levels
			, strength
			, X_life_per_4_dexterity, dexterity )
		{
			const int current_level = stats.GetStat( level );
			const int life_from_strength = stats.GetStat( strength ) / 2; // 2 point strength = 1 point max life
			const int life_from_levels = stats.GetStat( maximum_life_per_10_levels ) * current_level / 10 +
				stats.GetStat( life_per_level ) * ( current_level - 1 );

			return stats.GetStat( base_maximum_life ) +
				life_from_strength +
				life_from_levels +
				stats.GetStat( X_life_per_4_dexterity ) * int( stats.GetStat( dexterity ) / 4 );
		}

		VIRTUAL_STAT( combined_life_pluspercent
			, maximum_life_pluspercent
			, life_pluspercent_with_no_corrupted_equipped_items, number_of_equipped_corrupted_items )
		{
			return stats.GetStat( maximum_life_pluspercent ) + 
				( stats.GetStat( number_of_equipped_corrupted_items ) == 0 ? stats.GetStat( life_pluspercent_with_no_corrupted_equipped_items ) : 0 );
		}

		VIRTUAL_STAT( combined_life_pluspercent_final
			, max_life_pluspercent_final_for_minion
			, monster_life_pluspercent_final_from_map
			, monster_life_pluspercent_final_from_rarity
			, level_11_or_lower_life_pluspercent_final, level
			, level_33_or_lower_life_pluspercent_final
			, map_hidden_monster_life_pluspercent_final
			, level_1_to_40_life_pluspercent_final
			, level_41_to_57_life_pluspercent_final
			, level_58_to_70_life_pluspercent_final )
		{
			const int current_level = stats.GetStat( level );
			return Round( 100 *
				Scale( 100 + stats.GetStat( max_life_pluspercent_final_for_minion ) ) *
				Scale( 100 + stats.GetStat( monster_life_pluspercent_final_from_rarity ) ) *
				Scale( 100 + stats.GetStat( monster_life_pluspercent_final_from_map ) ) *
				( current_level <= 11 ? Scale( 100 + stats.GetStat( level_11_or_lower_life_pluspercent_final ) ) : 1 ) *
				( current_level <= 33 ? Scale( 100 + stats.GetStat( level_33_or_lower_life_pluspercent_final ) ) : 1 ) *
				Scale( 100 + stats.GetStat( map_hidden_monster_life_pluspercent_final ) ) *
				( ( current_level >= 1 && current_level <= 40 ) ? Scale( 100 + stats.GetStat( level_1_to_40_life_pluspercent_final ) ) : 1 ) *
				( ( current_level >= 41 && current_level <= 57 ) ? Scale( 100 + stats.GetStat( level_41_to_57_life_pluspercent_final ) ) : 1 ) *
				( ( current_level >= 58 && current_level <= 70 ) ? Scale( 100 + stats.GetStat( level_58_to_70_life_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( intermediary_maximum_life
			, total_base_maximum_life
			, combined_life_pluspercent
			, combined_life_pluspercent_final )
		{
			return std::max( Round( stats.GetStat( total_base_maximum_life ) * 
				Scale( 100 + stats.GetStat( combined_life_pluspercent ) ) *
				Scale( 100 + stats.GetStat( combined_life_pluspercent_final ) ) ), 0 );
		}

		VIRTUAL_STAT( intermediary_maximum_life_including_chaos_innoculation,
			keystone_chaos_inoculation,
			intermediary_maximum_life )
		{
			return stats.GetStat( keystone_chaos_inoculation ) ? 1 : stats.GetStat( intermediary_maximum_life );
		}

		VIRTUAL_STAT( maximum_mana,
			total_base_maximum_mana,
			combined_mana_pluspercent,
			combined_mana_pluspercent_final,
			no_mana )
		{
			if ( stats.GetStat( no_mana ) )
				return 0;

			return Round( stats.GetStat( total_base_maximum_mana ) *
				Scale( 100 + stats.GetStat( combined_mana_pluspercent ) ) *
				Scale( 100 + stats.GetStat( combined_mana_pluspercent_final ) ) );
		}