#include "VirtualStats.h"

#include "Common/Loaders/CEDefaultMonsterStats.h"
#include "Common/Loaders/CEDisplayMinionMonsterType.h"
#include "Common/Loaders/CEMonsterVarieties.h"
#include "Common/Loaders/CEStatsEnum.h"
#include "Common/Loaders/CEActiveSkillsEnum.h"
#include "Common/Loaders/CEGameConstants.h"
#include "Common/Items/WeaponClass.h"
#include "Common/Game/DamageCalculation.h"
#include "Common/Utility/Numeric.h"

#include "VirtualStatGetter.h"

namespace Stats
{

	VirtualStat virtual_stats[Loaders::StatsValues::NumStatsValues];

#define VIRTUAL_STAT( name, ...) \
	int Calculate_##name( const VirtualStatGetter& ); \
	Loaders::StatsValues::Stats name##_dependants[] = { __VA_ARGS__ }; \
	VirtualStatAdder name##_adder( name, std::begin( name##_dependants ), std::end( name##_dependants ), Calculate_##name ); \
	int Calculate_##name( const VirtualStatGetter& stats )

	namespace
	{
		struct VirtualStatAdder
		{
			VirtualStatAdder( const Loaders::StatsValues::Stats stat, const Loaders::StatsValues::Stats* dependants_begin, const Loaders::StatsValues::Stats* dependants_end, CalculateStat calculate_stat )
			{
				virtual_stats[stat].calculate_stat = calculate_stat;
				for ( auto dependant = dependants_begin; dependant != dependants_end; ++dependant )
					virtual_stats[*dependant].dependants.push_back( stat );
			}
		};

		inline int Round( const float f )
		{
			return int( f + ( f < 0.0f ? -0.5f : 0.5f ) );
		}

		inline int Round( const int f )
		{
			return f;
		}

		inline float Scale( const int i )
		{
			return std::max( 0.0f, i / 100.0f );
		}

		inline float Scalef( const float i )
		{
			return std::max( 0.0f, i / 100.0f );
		}

		inline int Divide( int a, int b )
		{
			return ( b == 0 ) ? 0 : ( a / b );
		}

		///takes an old % leech value and both applies the old leech scaling and converts value to permyriad
		inline int ScaleOldPercentLeech( int l )
		{
			return l * Loaders::GameConstants::GetDataRowByKey( L"Data/GameConstants.dat", L"OldLeechModsEffectiveness" )->Getvalue( );
		}
		///takes an old permyriad leech value and both applies only old leech scaling.
		inline int ScaleOldPermyriadLeech( int l )
		{
			return l * Loaders::GameConstants::GetDataRowByKey( L"Data/GameConstants.dat", L"OldLeechModsEffectiveness" )->Getvalue( ) / 100;
		}

		inline bool IsOneHandedMeleeWeaponClass( const Items::WeaponClass weapon_class )
		{
			return Items::IsMelee[weapon_class] && Items::IsOneHanded[weapon_class] && Items::IsWeapon[weapon_class];
		}

		using namespace Loaders::StatsValues;

		VIRTUAL_STAT( keystone_acrobatics_block_chance_pluspercent_final,
			keystone_acrobatics )
		{
			return ( stats.GetStat( keystone_acrobatics ) ? -30 : 0 );
		}

		VIRTUAL_STAT( block_percent,
			shield_block_percent, off_hand_weapon_type,
			staff_block_percent, main_hand_weapon_type,
			block_while_dual_wielding_claws_percent,
			block_while_dual_wielding_percent, is_dual_wielding,
			dual_wield_or_shield_block_percent,
			maximum_block_percent,
			additional_block_percent,
			monster_base_block_percent,
			keystone_acrobatics_block_chance_pluspercent_final,
			block_chance_pluspercent,
			with_bow_additional_block_percent,
			additional_staff_block_percent,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			local_no_block_chance )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );

			const bool has_shield = off_hand_weapon_index == Items::Shield;
			const bool has_staff = main_hand_weapon_index == Items::Staff;
			const bool has_bow = main_hand_weapon_index == Items::Bow;
			const bool dual_claws =
				main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Claw
				&& off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Claw;

			const float final_scale = Scale( 100 + stats.GetStat( block_chance_pluspercent ) ) *
				Scale( 100 + stats.GetStat( keystone_acrobatics_block_chance_pluspercent_final ) ); //currently two things, and one is multiplicative, so scale just has both multiply with each other.

			const int additional_block = stats.GetStat( additional_block_percent ) +
				( has_bow ? stats.GetStat( with_bow_additional_block_percent ) : 0 );

			if ( has_shield && !stats.GetStat( local_no_block_chance ) ) //This will need to change if that local stat is re-used on anything that's not a shield.
				return std::min( Round( ( stats.GetStat( shield_block_percent ) + stats.GetStat( dual_wield_or_shield_block_percent ) + additional_block + stats.GetStat( monster_base_block_percent ) ) * final_scale ), stats.GetStat( maximum_block_percent ) );
			if ( has_staff )
				return std::min( Round( ( stats.GetStat( staff_block_percent ) + stats.GetStat( additional_staff_block_percent ) + additional_block + stats.GetStat( monster_base_block_percent ) ) * final_scale ), stats.GetStat( maximum_block_percent ) );
			if ( stats.GetStat( is_dual_wielding ) )
				return std::min( Round( ( stats.GetStat( block_while_dual_wielding_percent ) + stats.GetStat( dual_wield_or_shield_block_percent ) + ( dual_claws ? stats.GetStat( block_while_dual_wielding_claws_percent ) : 0 ) + additional_block + stats.GetStat( monster_base_block_percent ) ) * final_scale ), stats.GetStat( maximum_block_percent ) );

			//if not in a situation that gives you block, get additional block only if you have some base block (currently only from being a monster that blocks)
			const auto monster_block = stats.GetStat( monster_base_block_percent );
			if ( monster_block )
				return std::min( Round( ( monster_block + additional_block ) * final_scale ), stats.GetStat( maximum_block_percent ) );

			return 0;
		}

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

		VIRTUAL_STAT( movement_velocity_pluspercent
			, base_movement_velocity_pluspercent
			, movement_velocity_pluspercent_per_frenzy_charge, current_frenzy_charges
			, ignore_armour_movement_penalties, from_armour_movement_speed_pluspercent
			, movement_velocity_pluspercent_when_on_low_life, on_low_life
			, movement_velocity_pluspercent_when_on_full_life, on_full_life
			, movement_velocity_pluspercent_final_for_minion
			, movement_velocity_pluspercent_while_cursed, curse_count
			, movement_velocity_pluspercent_per_ten_levels, level
			, movement_velocity_plus1percent_per_X_evasion_rating, evasion_rating
			, movement_velocity_pluspercent_per_shock, is_shocked
			, movement_velocity_pluspercent_per_10_rampage_stacks, current_rampage_stacks
			, movement_velocity_pluspercent_on_full_energy_shield, on_full_energy_shield
			, movement_speed_pluspercent_per_bloodline_speed_charge, current_bloodline_speed_charges
			, movement_velocity_pluspercent_while_phasing, virtual_phase_through_objects
			, movement_velocity_pluspercent_while_ignited, is_ignited
			, is_on_ground_lightning_shock, movement_velocity_pluspercent_when_on_shocked_ground
			, movement_speed_cannot_be_reduced_below_base
			, attack_and_movement_speed_pluspercent_with_her_blessing, have_her_blessing
			, movement_speed_pluspercent_while_not_affected_by_status_ailments, is_chilled, is_frozen
			, movement_velocity_while_not_hit_pluspercent
			, movement_speed_pluspercent_while_fortified, has_fortify
			, virtual_has_onslaught, onslaught_effect_pluspercent
			, labyrinth_arrow_movement_speed_pluspercent_final
			, modifiers_to_minion_movement_speed_also_affect_you, virtual_minion_movement_velocity_pluspercent
			, movement_speed_pluspercent_during_flask_effect, using_flask
			, enchantment_boots_movement_speed_pluspercent_when_not_hit_for_4_seconds, have_been_hit_in_past_4_seconds
			, movement_speed_pluspercent_if_used_a_warcry_recently, have_used_a_warcry_recently
			, movement_speed_pluspercent_if_pierced_recently, have_pierced_recently
			, movement_speed_pluspercent_if_enemy_killed_recently, have_killed_in_past_4_seconds
			, movement_speed_pluspercent_while_on_burning_chilled_shocked_ground, is_on_ground_fire_burn, is_on_ground_ice_chill
			, movement_velocity_while_spider_pluspercent, is_spider )
		{
			const int result = Round( ( stats.GetStat( base_movement_velocity_pluspercent ) +
				( ( stats.GetStat( is_ignited ) || stats.GetStat( is_frozen ) || stats.GetStat( is_chilled ) || stats.GetStat( is_shocked ) ) ? 0 : stats.GetStat( movement_speed_pluspercent_while_not_affected_by_status_ailments ) ) +
				( stats.GetStat( movement_velocity_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
				( stats.GetStat( movement_velocity_pluspercent_per_ten_levels ) * stats.GetStat( level ) / 10 ) +
				( stats.GetStat( ignore_armour_movement_penalties ) ? 0 : stats.GetStat( from_armour_movement_speed_pluspercent ) ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( movement_velocity_pluspercent_when_on_low_life ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_movement_speed_also_affect_you ) ? stats.GetStat( virtual_minion_movement_velocity_pluspercent ) : 0 ) +
				( stats.GetStat( on_full_life ) ? stats.GetStat( movement_velocity_pluspercent_when_on_full_life ) : 0 ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? stats.GetStat( movement_speed_pluspercent_if_enemy_killed_recently ) : 0 ) +
				( stats.GetStat( have_her_blessing ) ? stats.GetStat( attack_and_movement_speed_pluspercent_with_her_blessing ) : 0 ) +
				stats.GetStat( movement_velocity_while_not_hit_pluspercent ) +
				( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( movement_velocity_pluspercent_on_full_energy_shield ) : 0 ) +
				( stats.GetStat( is_spider ) ? stats.GetStat( movement_velocity_while_spider_pluspercent ) : 0 ) +
				( stats.GetStat( curse_count ) ? stats.GetStat( movement_velocity_pluspercent_while_cursed ) : 0 ) +
				( stats.GetStat( is_ignited ) ? stats.GetStat( movement_velocity_pluspercent_while_ignited ) : 0 ) +
				( Divide( stats.GetStat( evasion_rating ), stats.GetStat( movement_velocity_plus1percent_per_X_evasion_rating ) ) ) +
				( stats.GetStat( is_shocked ) ? stats.GetStat( movement_velocity_pluspercent_per_shock ) : 0 ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( movement_speed_pluspercent_while_fortified ) : 0 ) +
				( stats.GetStat( movement_velocity_pluspercent_per_10_rampage_stacks ) * ( stats.GetStat( current_rampage_stacks ) / 20 ) ) + //Actually per 20
				( stats.GetStat( movement_speed_pluspercent_per_bloodline_speed_charge ) * ( stats.GetStat( current_bloodline_speed_charges ) ) ) +
				( stats.GetStat( virtual_phase_through_objects ) ? stats.GetStat( movement_velocity_pluspercent_while_phasing ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? ( 20 * ( 100 + stats.GetStat( onslaught_effect_pluspercent ) ) / 100 ) : 0 ) + //onslaught grants 20% attack/cast/move speeds at base
				( stats.GetStat( is_on_ground_lightning_shock ) ? stats.GetStat( movement_velocity_pluspercent_when_on_shocked_ground ) : 0 ) +
				( stats.GetStat( is_on_ground_fire_burn ) || stats.GetStat( is_on_ground_lightning_shock ) || stats.GetStat( is_on_ground_ice_chill ) ? 
					stats.GetStat( movement_speed_pluspercent_while_on_burning_chilled_shocked_ground ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( movement_speed_pluspercent_during_flask_effect ) : 0 ) +
				( stats.GetStat( have_been_hit_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_movement_speed_pluspercent_when_not_hit_for_4_seconds ) ) +
				( stats.GetStat( have_used_a_warcry_recently ) ? stats.GetStat( movement_speed_pluspercent_if_used_a_warcry_recently ) : 0 ) +
				( stats.GetStat( have_pierced_recently ) ? stats.GetStat( movement_speed_pluspercent_if_pierced_recently ) : 0 ) +
				100 ) *
				Scale( 100 + stats.GetStat( labyrinth_arrow_movement_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( movement_velocity_pluspercent_final_for_minion ) )
				- 100 ); //final modifier is applied multiplicatively;  

			return ( stats.GetStat( movement_speed_cannot_be_reduced_below_base ) && result < 0 ) ? 0 : result;
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

		//these stats used to simplfy conversion of mana to ES
		VIRTUAL_STAT( total_base_maximum_mana,
			base_maximum_mana,
			mana_per_level, level,
			intelligence,
			no_mana,
			number_of_stackable_unique_jewels, X_mana_per_stackable_unique_jewel,
			X_mana_per_4_strength, strength )
		{
			if ( stats.GetStat( no_mana ) )
				return 0;

		const auto base_mana =
			stats.GetStat( base_maximum_mana ) +
			stats.GetStat( mana_per_level ) * ( stats.GetStat( level ) - 1 ) +
			stats.GetStat( intelligence ) * 0.5f + // 2 points int = 1 point max mana
			stats.GetStat( X_mana_per_stackable_unique_jewel ) * stats.GetStat( number_of_stackable_unique_jewels ) +
			stats.GetStat( X_mana_per_4_strength ) * int( stats.GetStat( strength ) / 4 );

			return Round( base_mana );
		}

		VIRTUAL_STAT( combined_mana_pluspercent,
			maximum_mana_pluspercent,
			maximum_mana_pluspercent_per_2percent_spell_block_chance,
			spell_block_percent )
		{
			return stats.GetStat( maximum_mana_pluspercent ) +
				stats.GetStat( maximum_mana_pluspercent_per_2percent_spell_block_chance ) * ( stats.GetStat( spell_block_percent ) / 2 );
		}

		VIRTUAL_STAT( combined_mana_pluspercent_final,
			monster_level_scale_maximum_mana_and_mana_cost_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( monster_level_scale_maximum_mana_and_mana_cost_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( maximum_energy_shield,
			total_base_maximum_energy_shield,
			combined_energy_shield_pluspercent,
			base_shield_maximum_energy_shield,
			combined_energy_shield_from_shield_pluspercent,
			combined_energy_shield_pluspercent_final,
			mana_percent_to_add_as_energy_shield, total_base_maximum_mana,
			combined_mana_pluspercent,
			combined_mana_pluspercent_final,
			maximum_life_percent_to_add_as_maximum_energy_shield, total_base_maximum_life,
			combined_life_pluspercent,
			combined_life_pluspercent_final,
			keystone_chaos_inoculation,
			no_energy_shield,
			minions_get_shield_stats_instead_of_you )
		{
			if ( stats.GetStat( no_energy_shield ) )
				return 0;

			const float energy_shield = (
				stats.GetStat( total_base_maximum_energy_shield ) * Scale( 100 + stats.GetStat( combined_energy_shield_pluspercent ) ) +
				( stats.GetStat( minions_get_shield_stats_instead_of_you ) ? 0 : stats.GetStat( base_shield_maximum_energy_shield ) * Scale( 100 + stats.GetStat( combined_energy_shield_from_shield_pluspercent ) ) )
				) * Scale( 100 + stats.GetStat( combined_energy_shield_pluspercent_final ) );


			float energy_shield_from_mana = 0.0f;
			if ( const auto percent_add_mana = stats.GetStat( mana_percent_to_add_as_energy_shield ) )
			{
				//convert to mana with applicable increases summed.
				energy_shield_from_mana = stats.GetStat( total_base_maximum_mana ) *
					Scale( percent_add_mana ) *
					Scale( 100 + stats.GetStat( combined_mana_pluspercent ) + stats.GetStat( combined_energy_shield_pluspercent ) ) *
					Scale( 100 + stats.GetStat( combined_energy_shield_pluspercent_final ) ) *
					Scale( 100 + stats.GetStat( combined_mana_pluspercent_final ) );
			}

			float energy_shield_from_life = 0.0f;
			if( const auto percent_add_life = stats.GetStat( maximum_life_percent_to_add_as_maximum_energy_shield ) )
			{
				if( stats.GetStat( keystone_chaos_inoculation ) )
				{
					energy_shield_from_life = 1.0f;
				}
				else
				{
					//convert to life with applicable increases summed.
					energy_shield_from_life = stats.GetStat( total_base_maximum_life ) *
						Scale( percent_add_life ) *
						Scale( 100 + stats.GetStat( combined_life_pluspercent ) + stats.GetStat( combined_energy_shield_pluspercent ) ) *
						Scale( 100 + stats.GetStat( combined_energy_shield_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( combined_life_pluspercent_final ) );
				}
			}

			return Round( energy_shield + energy_shield_from_mana + energy_shield_from_life );
		}

		VIRTUAL_STAT( total_base_maximum_energy_shield,
			maximum_life_percent_to_add_to_maximum_energy_shield, intermediary_maximum_life_including_chaos_innoculation,
			base_maximum_energy_shield,
			maximum_energy_shield_plus_per_5_strength, strength )
		{
			auto total_base_energy = stats.GetStat( base_maximum_energy_shield );

			//+x maximum Energy Shield per 5 Strength (stat)
			if ( const int energy_gain_from_strength = stats.GetStat( maximum_energy_shield_plus_per_5_strength ) )
			{
				total_base_energy += int( stats.GetStat( strength ) / 5 ) * energy_gain_from_strength;
			}

			const float from_life = stats.GetStat( intermediary_maximum_life_including_chaos_innoculation ) * Scale( stats.GetStat( maximum_life_percent_to_add_to_maximum_energy_shield ) );

			return Round( total_base_energy + from_life );
		}

		VIRTUAL_STAT( combined_energy_shield_pluspercent,
			maximum_energy_shield_pluspercent,
			intelligence,
			global_defences_pluspercent,
			energy_shield_pluspercent_per_10_strength, strength )
		{
			const int shield_increase_from_intelligence = Round( stats.GetStat( intelligence ) / 5.0f ); // 5 points int = 1% increased energy shield

			return stats.GetStat( maximum_energy_shield_pluspercent ) + 
				shield_increase_from_intelligence + 
				stats.GetStat( global_defences_pluspercent ) +
				stats.GetStat( energy_shield_pluspercent_per_10_strength ) * int( stats.GetStat( strength ) / 10 );
		}

		VIRTUAL_STAT( combined_energy_shield_from_shield_pluspercent,
			combined_energy_shield_pluspercent,
			shield_maximum_energy_shield_pluspercent,
			shield_armour_pluspercent )
		{
			return stats.GetStat( combined_energy_shield_pluspercent ) + stats.GetStat( shield_maximum_energy_shield_pluspercent ) + stats.GetStat( shield_armour_pluspercent );
		}

		VIRTUAL_STAT( combined_energy_shield_pluspercent_final,
			energy_shield_pluspercent_final_for_minion,
			chaos_inoculation_keystone_energy_shield_pluspercent_final,
			keystone_acrobatics_energy_shield_pluspercent_final,
			additional_player_bonus_maximum_energy_shield_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( energy_shield_pluspercent_final_for_minion ) ) *
				Scale( 100 + stats.GetStat( chaos_inoculation_keystone_energy_shield_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( keystone_acrobatics_energy_shield_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( additional_player_bonus_maximum_energy_shield_pluspercent_final ) )
				- 100 );
		}

		//used both for life regen, and converted ES regen, to reduce duplication
		VIRTUAL_STAT( total_base_life_regeneration_rate_per_minute,
			base_life_regeneration_rate_per_minute,
			life_regeneration_rate_per_minute_per_level, level,
			no_life_regeneration,
			keystone_vampirism,
			cannot_recover_life,
			life_regeneration_rate_pluspercent,
			life_regeneration_rate_pluspercent_while_es_full, on_full_energy_shield,
			virtual_minion_life_regeneration_per_minute, modifiers_to_minion_life_regeneration_also_affect_you,
			life_regen_per_minute_per_endurance_charge, current_endurance_charges )
		{
			if ( stats.GetStat( no_life_regeneration ) || stats.GetStat( keystone_vampirism ) || stats.GetStat( cannot_recover_life ) )
				return 0;

			return Round(
				( stats.GetStat( base_life_regeneration_rate_per_minute ) +
					( stats.GetStat( modifiers_to_minion_life_regeneration_also_affect_you ) ? stats.GetStat( virtual_minion_life_regeneration_per_minute ) : 0 ) +
					( stats.GetStat( life_regeneration_rate_per_minute_per_level ) * stats.GetStat( level ) ) +
					( stats.GetStat( life_regen_per_minute_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) )
					) *
				Scale( 100 + stats.GetStat( life_regeneration_rate_pluspercent ) + ( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( life_regeneration_rate_pluspercent_while_es_full ) : 0 ) )
				);
		}

		//used both for life regen, and converted ES regen, to reduce duplication
		VIRTUAL_STAT( total_base_life_regeneration_rate_per_minute_percent,
			life_regeneration_rate_per_minute_percent,
			life_regeneration_rate_per_minute_percent_per_endurance_charge, current_endurance_charges,
			life_regeneration_rate_per_minute_percent_per_frenzy_charge, current_frenzy_charges,
			life_regeneration_rate_per_minute_percent_when_on_low_life, on_low_life,
			life_regeneration_per_minute_percent_while_fortified, has_fortify,
			life_regeneration_per_minute_percent_while_frozen, is_frozen,
			no_life_regeneration,
			keystone_vampirism,
			cannot_recover_life,
			life_regeneration_rate_pluspercent,
			base_life_regeneration_per_minute_percent_while_in_lava, in_lava,
			life_regeneration_rate_pluspercent_while_es_full, on_full_energy_shield,
			is_on_ground_ice_chill, life_regeneration_rate_per_minute_percent_when_on_chilled_ground,
			number_of_active_totems, life_regenerate_rate_per_second_percent_while_totem_active,
			life_regeneration_rate_per_minute_percent_if_taunted_an_enemy_recently, have_taunted_an_enemy_recently )
		{
			if ( stats.GetStat( no_life_regeneration ) || stats.GetStat( keystone_vampirism ) || stats.GetStat( cannot_recover_life ) )
				return 0;

			return Round(
				( stats.GetStat( life_regeneration_rate_per_minute_percent ) +
					stats.GetStat( life_regeneration_rate_per_minute_percent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) +
					stats.GetStat( life_regeneration_rate_per_minute_percent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
					( stats.GetStat( on_low_life ) ? stats.GetStat( life_regeneration_rate_per_minute_percent_when_on_low_life ) : 0 ) +
					( stats.GetStat( in_lava ) ? stats.GetStat( base_life_regeneration_per_minute_percent_while_in_lava ) : 0 ) +
					( stats.GetStat( has_fortify ) ? stats.GetStat( life_regeneration_per_minute_percent_while_fortified ) : 0 ) +
					( stats.GetStat( number_of_active_totems ) ? stats.GetStat( life_regenerate_rate_per_second_percent_while_totem_active ) * 60 : 0 ) +
					( stats.GetStat( have_taunted_an_enemy_recently ) ? stats.GetStat( life_regeneration_rate_per_minute_percent_if_taunted_an_enemy_recently ) : 0 ) +
					( stats.GetStat( is_frozen ) ? stats.GetStat( life_regeneration_per_minute_percent_while_frozen ) : 0 ) ) *
				Scale( 100 + stats.GetStat( life_regeneration_rate_pluspercent ) +
					( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( life_regeneration_rate_pluspercent_while_es_full ) : 0 ) ) +
				( stats.GetStat( is_on_ground_ice_chill ) ? stats.GetStat( life_regeneration_rate_per_minute_percent_when_on_chilled_ground ) : 0 ) );
		}

		VIRTUAL_STAT( life_regeneration_rate_per_minute,
			total_base_life_regeneration_rate_per_minute,
			total_base_life_regeneration_rate_per_minute_percent, maximum_life,
			minion_life_regeneration_rate_per_minute_percent, modifiers_to_minion_life_regeneration_also_affect_you,
			regenerate_energy_shield_instead_of_life,
			life_regeneration_per_minute_with_no_corrupted_equipped_items, number_of_equipped_corrupted_items )
		{
			if ( stats.GetStat( regenerate_energy_shield_instead_of_life ) )
				return 0;

			//regeneration
			const float regeneration_from_percentage_of_total_life =
				Scale( stats.GetStat( total_base_life_regeneration_rate_per_minute_percent ) +
					( stats.GetStat( modifiers_to_minion_life_regeneration_also_affect_you ) ? stats.GetStat( minion_life_regeneration_rate_per_minute_percent ) : 0 ) )
				* stats.GetStat( maximum_life );
			const float total_regeneration =
				stats.GetStat( total_base_life_regeneration_rate_per_minute ) +
				regeneration_from_percentage_of_total_life +
				( stats.GetStat( number_of_equipped_corrupted_items ) == 0 ? stats.GetStat( life_regeneration_per_minute_with_no_corrupted_equipped_items ) : 0 );

			return Round( total_regeneration );
		}

		VIRTUAL_STAT( life_recovery_per_minute,
			life_recovery_per_minute_from_leech,
			life_regeneration_rate_per_minute,
			base_life_recovery_per_minute,
			life_recovery_speed_pluspercent_final_from_map,
			keystone_vampirism,
			cannot_recover_life,
			life_recovery_rate_pluspercent,
			maximum_life_leech_rate_percent_per_minute, maximum_life,
			total_healing_from_damage_taken_per_minute )
		{
			if ( stats.GetStat( cannot_recover_life ) )
				return 0;

			// Set cap to life leech rate maximum_life_leech_rate_percent_per_minute
			const int maximum_life_leech_rate = static_cast< int >( Scale( stats.GetStat( maximum_life_leech_rate_percent_per_minute ) ) * stats.GetStat( maximum_life ) );
			const auto leech = std::min( stats.GetStat( life_recovery_per_minute_from_leech ), maximum_life_leech_rate );
			const auto recovery = stats.GetStat( base_life_recovery_per_minute );

			if ( stats.GetStat( keystone_vampirism ) )
				return Round( ( leech + recovery ) * Scale( 100 + stats.GetStat( life_recovery_speed_pluspercent_final_from_map ) ) );

			const auto regeneration = stats.GetStat( life_regeneration_rate_per_minute );

			const auto damage_healing = stats.GetStat( total_healing_from_damage_taken_per_minute );

			return Round( ( leech + recovery + regeneration ) * Scale( 100 + stats.GetStat( life_recovery_rate_pluspercent ) ) * Scale( 100 + stats.GetStat( life_recovery_speed_pluspercent_final_from_map ) ) + damage_healing );
		}

		VIRTUAL_STAT( mana_regeneration_rate_per_minute,
			mana_regeneration_rate_per_minute_percent,
			mana_regeneration_rate_per_minute_percent_per_power_charge, current_power_charges,
			mana_regeneration_rate_pluspercent_per_power_charge,
			maximum_mana,
			base_mana_regeneration_rate_per_minute,
			mana_regeneration_rate_pluspercent,
			no_mana_regeneration,
			cannot_leech_or_regenerate_mana,
			mana_regeneration_rate_pluspercent_during_flask_effect, using_flask,
			es_and_mana_regeneration_rate_per_minute_percent_while_on_consecrated_ground, on_consecrated_ground,
			mana_regeneration_rate_pluspercent_while_phasing, virtual_phase_through_objects )
		{
			if ( stats.GetStat( no_mana_regeneration ) || stats.GetStat( cannot_leech_or_regenerate_mana ) )
				return 0;

			const float regeneration_from_percentage_of_total_mana =
				Scale( stats.GetStat( mana_regeneration_rate_per_minute_percent ) + 
					stats.GetStat( mana_regeneration_rate_per_minute_percent_per_power_charge ) * stats.GetStat( current_power_charges ) + 
					( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( es_and_mana_regeneration_rate_per_minute_percent_while_on_consecrated_ground ) : 0 ) 
					) * stats.GetStat( maximum_mana );
			
			const float overall_scaling = Scale( 100 +
				stats.GetStat( mana_regeneration_rate_pluspercent ) +
				stats.GetStat( mana_regeneration_rate_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) +
				( stats.GetStat( virtual_phase_through_objects ) ? stats.GetStat( mana_regeneration_rate_pluspercent_while_phasing ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( mana_regeneration_rate_pluspercent_during_flask_effect ) : 0 ) );
			const auto total_regeneration = ( regeneration_from_percentage_of_total_mana + stats.GetStat( base_mana_regeneration_rate_per_minute ) ) * overall_scaling;

			return Round( total_regeneration );
		}

		VIRTUAL_STAT( mana_recovery_per_minute,
			mana_recovery_per_minute_from_leech,
			mana_regeneration_rate_per_minute,
			base_mana_recovery_per_minute,
			mana_recovery_speed_pluspercent_final_from_map,
			mana_recovery_rate_pluspercent,
			maximum_mana_leech_rate_percent_per_minute, maximum_mana )
		{
			const int maximum_mana_leech_rate = static_cast< int >( Scale( stats.GetStat( maximum_mana_leech_rate_percent_per_minute ) ) * stats.GetStat( maximum_mana ) );
			const auto leech = std::min( stats.GetStat( mana_recovery_per_minute_from_leech ), maximum_mana_leech_rate );
			const auto recovery = stats.GetStat( base_mana_recovery_per_minute );
			const auto regeneration = stats.GetStat( mana_regeneration_rate_per_minute );

			return Round( ( leech + recovery + regeneration ) * Scale( 100 + stats.GetStat( mana_recovery_rate_pluspercent ) ) * Scale( 100 + stats.GetStat( mana_recovery_speed_pluspercent_final_from_map ) ) );
		}

		//this is natural ES recharge, suppressed by the cooldown
		VIRTUAL_STAT( energy_shield_recharge_rate_per_minute,
			energy_shield_recharge_rate_per_minute_percent,
			maximum_energy_shield,
			virtual_energy_shield_recharge_rate_pluspercent,
			no_energy_shield_recharge_or_regeneration,
			energy_shield_recovery_speed_pluspercent_final_from_map,
			energy_shield_recovery_rate_pluspercent )
		{
			if ( stats.GetStat( no_energy_shield_recharge_or_regeneration ) )
				return 0;

			const float recharge_from_percentage_of_total_energy_shield = Scale( stats.GetStat( energy_shield_recharge_rate_per_minute_percent ) ) * stats.GetStat( maximum_energy_shield );
			const float overall_scaling = Scale( 100
				+ stats.GetStat( virtual_energy_shield_recharge_rate_pluspercent )
				+ stats.GetStat( energy_shield_recovery_rate_pluspercent ) );

			return Round( ( recharge_from_percentage_of_total_energy_shield ) * overall_scaling * Scale( 100 + stats.GetStat( energy_shield_recovery_speed_pluspercent_final_from_map ) ) );
		}

		//this is constant regeneration, not suppressed by cooldown.
		VIRTUAL_STAT( energy_shield_regeneration_rate_per_minute,
			base_energy_shield_regeneration_rate_per_minute,
			base_energy_shield_regeneration_rate_per_minute_percent, maximum_energy_shield,
			regenerate_energy_shield_instead_of_life,
			total_base_life_regeneration_rate_per_minute,
			total_base_life_regeneration_rate_per_minute_percent,
			no_energy_shield_recharge_or_regeneration,
			energy_shield_regeneration_percent_per_minute_while_shocked, is_shocked,
			energy_shield_recharge_rate_per_minute_with_all_corrupted_equipped_items, number_of_equipped_items, number_of_equipped_corrupted_items, 
			energy_shield_regeneration_rate_per_minute_percent_while_on_low_life, on_low_life,
			es_and_mana_regeneration_rate_per_minute_percent_while_on_consecrated_ground, on_consecrated_ground )
		{
			if ( stats.GetStat( no_energy_shield_recharge_or_regeneration ) )
				return 0;

			int regeneration = stats.GetStat( base_energy_shield_regeneration_rate_per_minute ) + 
				( stats.GetStat( number_of_equipped_corrupted_items ) == stats.GetStat( number_of_equipped_items ) ?
				stats.GetStat( energy_shield_recharge_rate_per_minute_with_all_corrupted_equipped_items ) : 0 );

			int regeneration_percent = stats.GetStat( base_energy_shield_regeneration_rate_per_minute_percent ) +
				( stats.GetStat( is_shocked ) ? stats.GetStat( energy_shield_regeneration_percent_per_minute_while_shocked ) : 0 ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( energy_shield_regeneration_rate_per_minute_percent_while_on_low_life ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( es_and_mana_regeneration_rate_per_minute_percent_while_on_consecrated_ground ) : 0 );

			if ( stats.GetStat( regenerate_energy_shield_instead_of_life ) )
			{
				regeneration_percent += stats.GetStat( total_base_life_regeneration_rate_per_minute_percent );
				regeneration += stats.GetStat( total_base_life_regeneration_rate_per_minute );
			}

			//total
			return Round( regeneration + Scale( regeneration_percent ) * stats.GetStat( maximum_energy_shield ) );
		}

		//this is constant recovery, not suppressed by cooldown.
		VIRTUAL_STAT( energy_shield_recovery_per_minute
			, energy_shield_recovery_per_minute_from_leech
			, energy_shield_regeneration_rate_per_minute
			, energy_shield_recovery_speed_pluspercent_final_from_map
			, energy_shield_recovery_rate_pluspercent
			, maximum_life_leech_rate_percent_per_minute
			, leech_energy_shield_instead_of_life, maximum_energy_shield )
		{
			const bool leeching_es_instead_of_life = !!stats.GetStat( leech_energy_shield_instead_of_life );
			const int maximum_shield_leech_rate = static_cast< int >( Scale( stats.GetStat( maximum_life_leech_rate_percent_per_minute ) ) * stats.GetStat( maximum_energy_shield ) );

			// If this is applying es leech instead of life, apply life leech cap to this leech rate
			const auto base_leech_rate = stats.GetStat( energy_shield_recovery_per_minute_from_leech );
			const auto leech = leeching_es_instead_of_life ? std::min( base_leech_rate, maximum_shield_leech_rate ) : base_leech_rate;
			const auto regeneration = stats.GetStat( energy_shield_regeneration_rate_per_minute );

			return Round( ( leech + regeneration ) * Scale( 100 + stats.GetStat( energy_shield_recovery_rate_pluspercent ) ) * Scale( 100 + stats.GetStat( energy_shield_recovery_speed_pluspercent_final_from_map ) ) );
		}

		///
		/// Summed damage values for character sheet
		///

		VIRTUAL_STAT( spell_minimum_total_damage,
			spell_minimum_physical_damage,
			spell_minimum_fire_damage,
			spell_minimum_cold_damage,
			spell_minimum_lightning_damage,
			spell_minimum_chaos_damage )
		{
			return stats.GetStat( spell_minimum_physical_damage ) +
				stats.GetStat( spell_minimum_fire_damage ) +
				stats.GetStat( spell_minimum_cold_damage ) +
				stats.GetStat( spell_minimum_lightning_damage ) +
				stats.GetStat( spell_minimum_chaos_damage );
		}

		VIRTUAL_STAT( spell_maximum_total_damage,
			spell_maximum_physical_damage,
			spell_maximum_fire_damage,
			spell_maximum_cold_damage,
			spell_maximum_lightning_damage,
			spell_maximum_chaos_damage )
		{
			return stats.GetStat( spell_maximum_physical_damage ) +
				stats.GetStat( spell_maximum_fire_damage ) +
				stats.GetStat( spell_maximum_cold_damage ) +
				stats.GetStat( spell_maximum_lightning_damage ) +
				stats.GetStat( spell_maximum_chaos_damage );
		}

		VIRTUAL_STAT( secondary_minimum_total_damage,
			secondary_minimum_physical_damage,
			secondary_minimum_fire_damage,
			secondary_minimum_cold_damage,
			secondary_minimum_lightning_damage,
			secondary_minimum_chaos_damage )
		{
			return stats.GetStat( secondary_minimum_physical_damage ) +
				stats.GetStat( secondary_minimum_fire_damage ) +
				stats.GetStat( secondary_minimum_cold_damage ) +
				stats.GetStat( secondary_minimum_lightning_damage ) +
				stats.GetStat( secondary_minimum_chaos_damage );
		}

		VIRTUAL_STAT( secondary_maximum_total_damage,
			secondary_maximum_physical_damage,
			secondary_maximum_fire_damage,
			secondary_maximum_cold_damage,
			secondary_maximum_lightning_damage,
			secondary_maximum_chaos_damage )
		{
			return stats.GetStat( secondary_maximum_physical_damage ) +
				stats.GetStat( secondary_maximum_fire_damage ) +
				stats.GetStat( secondary_maximum_cold_damage ) +
				stats.GetStat( secondary_maximum_lightning_damage ) +
				stats.GetStat( secondary_maximum_chaos_damage );
		}

		VIRTUAL_STAT( main_hand_minimum_total_damage, main_hand_minimum_physical_damage, main_hand_minimum_fire_damage, main_hand_minimum_cold_damage, main_hand_minimum_lightning_damage, main_hand_minimum_chaos_damage )
		{
			return stats.GetStat( main_hand_minimum_physical_damage ) +
				stats.GetStat( main_hand_minimum_fire_damage ) +
				stats.GetStat( main_hand_minimum_cold_damage ) +
				stats.GetStat( main_hand_minimum_lightning_damage ) +
				stats.GetStat( main_hand_minimum_chaos_damage );
		}

		VIRTUAL_STAT( main_hand_maximum_total_damage, main_hand_maximum_physical_damage, main_hand_maximum_fire_damage, main_hand_maximum_cold_damage, main_hand_maximum_lightning_damage, main_hand_maximum_chaos_damage )
		{
			return stats.GetStat( main_hand_maximum_physical_damage ) +
				stats.GetStat( main_hand_maximum_fire_damage ) +
				stats.GetStat( main_hand_maximum_cold_damage ) +
				stats.GetStat( main_hand_maximum_lightning_damage ) +
				stats.GetStat( main_hand_maximum_chaos_damage );
		}

		VIRTUAL_STAT( off_hand_minimum_total_damage, off_hand_minimum_physical_damage, off_hand_minimum_fire_damage, off_hand_minimum_cold_damage, off_hand_minimum_lightning_damage, off_hand_minimum_chaos_damage )
		{
			return stats.GetStat( off_hand_minimum_physical_damage ) +
				stats.GetStat( off_hand_minimum_fire_damage ) +
				stats.GetStat( off_hand_minimum_cold_damage ) +
				stats.GetStat( off_hand_minimum_lightning_damage ) +
				stats.GetStat( off_hand_minimum_chaos_damage );
		}

		VIRTUAL_STAT( off_hand_maximum_total_damage, off_hand_maximum_physical_damage, off_hand_maximum_fire_damage, off_hand_maximum_cold_damage, off_hand_maximum_lightning_damage, off_hand_maximum_chaos_damage )
		{
			return stats.GetStat( off_hand_maximum_physical_damage ) +
				stats.GetStat( off_hand_maximum_fire_damage ) +
				stats.GetStat( off_hand_maximum_cold_damage ) +
				stats.GetStat( off_hand_maximum_lightning_damage ) +
				stats.GetStat( off_hand_maximum_chaos_damage );
		}

		///
		/// Total damage stats
		///

		// Spell Damage

		VIRTUAL_STAT( spell_minimum_physical_damage,
			//base damage values
			spell_total_minimum_base_physical_damage,
			spell_total_minimum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_physical_damage )
		{
			if ( stats.GetStat( deal_no_spell_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( spell_total_minimum_base_physical_damage ) +
				( stats.GetStat( spell_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( spell_maximum_physical_damage,
			//base damage values
			spell_total_maximum_base_physical_damage,
			spell_total_maximum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_physical_damage )
		{
			if ( stats.GetStat( deal_no_spell_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( spell_total_maximum_base_physical_damage ) +
				( stats.GetStat( spell_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( spell_minimum_fire_damage,
			//base damage
			spell_total_minimum_base_fire_damage,
			spell_total_minimum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_fire_damage_pluspercent,
			combined_spell_fire_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			spell_total_minimum_base_physical_damage,
			spell_total_minimum_added_physical_damage,
			spell_total_minimum_base_cold_damage,
			spell_total_minimum_added_cold_damage,
			spell_total_minimum_base_lightning_damage,
			spell_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			combined_spell_cold_damage_pluspercent,
			combined_spell_cold_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_fire_damage )
		{
			if ( stats.GetStat( deal_no_spell_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( spell_total_minimum_base_fire_damage ) +
				( stats.GetStat( spell_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_minimum_base_physical_damage ) +
				( stats.GetStat( spell_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( spell_total_minimum_base_cold_damage ) +
				( stats.GetStat( spell_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_minimum_base_lightning_damage ) +
				( stats.GetStat( spell_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_fire_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_maximum_fire_damage,
			//base damage
			spell_total_maximum_base_fire_damage,
			spell_total_maximum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_fire_damage_pluspercent,
			combined_spell_fire_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			spell_total_maximum_base_physical_damage,
			spell_total_maximum_added_physical_damage,
			spell_total_maximum_base_cold_damage,
			spell_total_maximum_added_cold_damage,
			spell_total_maximum_base_lightning_damage,
			spell_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			combined_spell_cold_damage_pluspercent,
			combined_spell_cold_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_fire_damage )
		{
			if ( stats.GetStat( deal_no_spell_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( spell_total_maximum_base_fire_damage ) +
				( stats.GetStat( spell_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_maximum_base_physical_damage ) +
				( stats.GetStat( spell_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( spell_total_maximum_base_cold_damage ) +
				( stats.GetStat( spell_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_maximum_base_lightning_damage ) +
				( stats.GetStat( spell_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_fire_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_minimum_cold_damage,
			//base damage
			spell_total_minimum_base_cold_damage,
			spell_total_minimum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_cold_damage_pluspercent,
			combined_spell_cold_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			spell_total_minimum_base_physical_damage,
			spell_total_minimum_added_physical_damage,
			spell_total_minimum_base_lightning_damage,
			spell_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_cold_damage )
		{
			if ( stats.GetStat( deal_no_spell_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( spell_total_minimum_base_cold_damage ) +
				( stats.GetStat( spell_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_minimum_base_physical_damage ) +
				( stats.GetStat( spell_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_minimum_base_lightning_damage ) +
				( stats.GetStat( spell_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_cold_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_maximum_cold_damage,
			//base damage
			spell_total_maximum_base_cold_damage,
			spell_total_maximum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_cold_damage_pluspercent,
			combined_spell_cold_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			spell_total_maximum_base_physical_damage,
			spell_total_maximum_added_physical_damage,
			spell_total_maximum_base_lightning_damage,
			spell_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			deal_no_spell_cold_damage )
		{
			if ( stats.GetStat( deal_no_spell_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( spell_total_maximum_base_cold_damage ) +
				( stats.GetStat( spell_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_maximum_base_physical_damage ) +
				( stats.GetStat( spell_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_maximum_base_lightning_damage ) +
				( stats.GetStat( spell_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_cold_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_minimum_lightning_damage,
			//base damage
			spell_total_minimum_base_lightning_damage,
			spell_total_minimum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			spell_total_minimum_base_physical_damage,
			spell_total_minimum_added_physical_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_lightning_damage )
		{
			if ( stats.GetStat( deal_no_spell_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_minimum_base_lightning_damage ) +
				( stats.GetStat( spell_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_minimum_base_physical_damage ) +
				( stats.GetStat( spell_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_lightning_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_maximum_lightning_damage,
			//base damage
			spell_total_maximum_base_lightning_damage,
			spell_total_maximum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			spell_total_maximum_base_physical_damage,
			spell_total_maximum_added_physical_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_lightning_damage )
		{
			if ( stats.GetStat( deal_no_spell_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_maximum_base_lightning_damage ) +
				( stats.GetStat( spell_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_maximum_base_physical_damage ) +
				( stats.GetStat( spell_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_lightning_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_minimum_chaos_damage,
			//base damage
			spell_total_minimum_base_chaos_damage,
			spell_total_minimum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_chaos_damage_pluspercent,
			combined_spell_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			spell_total_minimum_base_physical_damage,
			spell_total_minimum_added_physical_damage,
			spell_total_minimum_base_fire_damage,
			spell_total_minimum_added_fire_damage,
			spell_total_minimum_base_cold_damage,
			spell_total_minimum_added_cold_damage,
			spell_total_minimum_base_lightning_damage,
			spell_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			combined_spell_fire_damage_pluspercent,
			combined_spell_fire_damage_pluspercent_final,
			combined_spell_cold_damage_pluspercent,
			combined_spell_cold_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_chaos_damage )
		{
			if ( stats.GetStat( deal_no_spell_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( spell_total_minimum_base_chaos_damage ) +
				( stats.GetStat( spell_total_minimum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_minimum_base_physical_damage ) +
				( stats.GetStat( spell_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( spell_total_minimum_base_fire_damage ) +
				( stats.GetStat( spell_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( spell_total_minimum_base_cold_damage ) +
				( stats.GetStat( spell_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_minimum_base_lightning_damage ) +
				( stats.GetStat( spell_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		VIRTUAL_STAT( spell_maximum_chaos_damage,
			//base damage
			spell_total_maximum_base_chaos_damage,
			spell_total_maximum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_spell_all_damage_pluspercent,
			combined_spell_all_damage_pluspercent_final,
			combined_spell_chaos_damage_pluspercent,
			combined_spell_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			spell_total_maximum_base_physical_damage,
			spell_total_maximum_added_physical_damage,
			spell_total_maximum_base_fire_damage,
			spell_total_maximum_added_fire_damage,
			spell_total_maximum_base_cold_damage,
			spell_total_maximum_added_cold_damage,
			spell_total_maximum_base_lightning_damage,
			spell_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_spell_physical_damage_pluspercent,
			combined_spell_physical_damage_pluspercent_final,
			combined_spell_fire_damage_pluspercent,
			combined_spell_fire_damage_pluspercent_final,
			combined_spell_cold_damage_pluspercent,
			combined_spell_cold_damage_pluspercent_final,
			combined_spell_lightning_damage_pluspercent,
			combined_spell_lightning_damage_pluspercent_final,
			combined_spell_elemental_damage_pluspercent,
			combined_spell_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_spell_chaos_damage )
		{
			if ( stats.GetStat( deal_no_spell_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( spell_total_maximum_base_chaos_damage ) +
				( stats.GetStat( spell_total_maximum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( spell_total_maximum_base_physical_damage ) +
				( stats.GetStat( spell_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( spell_total_maximum_base_fire_damage ) +
				( stats.GetStat( spell_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( spell_total_maximum_base_cold_damage ) +
				( stats.GetStat( spell_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( spell_total_maximum_base_lightning_damage ) +
				( stats.GetStat( spell_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_spell_all_damage_pluspercent ) + stats.GetStat( combined_spell_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_spell_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_lightning_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_cold_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_spell_fire_damage_pluspercent ) + stats.GetStat( combined_spell_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_spell_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_spell_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		// Spell base damage

		VIRTUAL_STAT( spell_total_minimum_base_physical_damage,
			spell_minimum_base_physical_damage )
		{
			return stats.GetStat( spell_minimum_base_physical_damage );
		}

		VIRTUAL_STAT( spell_total_maximum_base_physical_damage,
			spell_maximum_base_physical_damage )
		{
			return stats.GetStat( spell_maximum_base_physical_damage );
		}

		VIRTUAL_STAT( spell_total_minimum_base_fire_damage,
			spell_minimum_base_fire_damage,
			spell_minimum_base_fire_damage_per_endurance_charge, current_endurance_charges )
		{
			return stats.GetStat( spell_minimum_base_fire_damage ) + stats.GetStat( spell_minimum_base_fire_damage_per_endurance_charge ) * stats.GetStat( current_endurance_charges );
		}

		VIRTUAL_STAT( spell_total_maximum_base_fire_damage,
			spell_maximum_base_fire_damage,
			spell_maximum_base_fire_damage_per_endurance_charge, current_endurance_charges )
		{
			return stats.GetStat( spell_maximum_base_fire_damage ) + stats.GetStat( spell_maximum_base_fire_damage_per_endurance_charge ) * stats.GetStat( current_endurance_charges );
		}

		VIRTUAL_STAT( spell_total_minimum_base_cold_damage,
			spell_minimum_base_cold_damage,
			spell_minimum_base_cold_damage_per_frenzy_charge, current_frenzy_charges,
			spell_minimum_base_cold_damage_plus_per_10_intelligence, intelligence )
		{
			return stats.GetStat( spell_minimum_base_cold_damage ) + stats.GetStat( spell_minimum_base_cold_damage_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				( stats.GetStat( spell_minimum_base_cold_damage_plus_per_10_intelligence ) * ( stats.GetStat( intelligence ) / 10 ) );
		}

		VIRTUAL_STAT( spell_total_maximum_base_cold_damage,
			spell_maximum_base_cold_damage,
			spell_maximum_base_cold_damage_per_frenzy_charge, current_frenzy_charges,
			spell_maximum_base_cold_damage_plus_per_10_intelligence, intelligence )
		{
			return stats.GetStat( spell_maximum_base_cold_damage ) + stats.GetStat( spell_maximum_base_cold_damage_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				( stats.GetStat( spell_maximum_base_cold_damage_plus_per_10_intelligence ) * ( stats.GetStat( intelligence ) / 10 ) );
		}

		VIRTUAL_STAT( spell_total_minimum_base_lightning_damage,
			spell_minimum_base_lightning_damage,
			spell_minimum_base_lightning_damage_per_power_charge, current_power_charges )
		{
			return stats.GetStat( spell_minimum_base_lightning_damage ) + stats.GetStat( spell_minimum_base_lightning_damage_per_power_charge ) * stats.GetStat( current_power_charges );
		}

		VIRTUAL_STAT( spell_total_maximum_base_lightning_damage,
			spell_maximum_base_lightning_damage,
			spell_maximum_base_lightning_damage_per_power_charge, current_power_charges )
		{
			return stats.GetStat( spell_maximum_base_lightning_damage ) + stats.GetStat( spell_maximum_base_lightning_damage_per_power_charge ) * stats.GetStat( current_power_charges );
		}

		VIRTUAL_STAT( spell_total_minimum_base_chaos_damage,
			spell_minimum_base_chaos_damage )
		{
			return stats.GetStat( spell_minimum_base_chaos_damage );
		}

		VIRTUAL_STAT( spell_total_maximum_base_chaos_damage,
			spell_maximum_base_chaos_damage )
		{
			return stats.GetStat( spell_maximum_base_chaos_damage );
		}

		//Spell added damage

		VIRTUAL_STAT( spell_total_minimum_added_physical_damage,
			global_total_minimum_added_physical_damage,
			spell_minimum_added_physical_damage )
		{
			return stats.GetStat( global_total_minimum_added_physical_damage ) + stats.GetStat( spell_minimum_added_physical_damage );
		}

		VIRTUAL_STAT( spell_total_maximum_added_physical_damage,
			global_total_maximum_added_physical_damage,
			spell_maximum_added_physical_damage )
		{
			return stats.GetStat( global_total_maximum_added_physical_damage ) + stats.GetStat( spell_maximum_added_physical_damage );
		}

		VIRTUAL_STAT( spell_total_minimum_added_fire_damage,
			global_total_minimum_added_fire_damage,
			spell_minimum_added_fire_damage,
			number_of_active_buffs, minimum_added_fire_damage_per_active_buff,
			minimum_added_fire_spell_damage_per_active_buff )
		{
			return stats.GetStat( global_total_minimum_added_fire_damage ) + stats.GetStat( spell_minimum_added_fire_damage ) +
				( stats.GetStat( number_of_active_buffs ) *
					( stats.GetStat( minimum_added_fire_damage_per_active_buff ) + stats.GetStat( minimum_added_fire_spell_damage_per_active_buff ) ) );
		}

		VIRTUAL_STAT( spell_total_maximum_added_fire_damage,
			global_total_maximum_added_fire_damage,
			spell_maximum_added_fire_damage,
			number_of_active_buffs, maximum_added_fire_damage_per_active_buff,
			maximum_added_fire_spell_damage_per_active_buff )
		{
			return stats.GetStat( global_total_maximum_added_fire_damage ) + stats.GetStat( spell_maximum_added_fire_damage ) +
				( stats.GetStat( number_of_active_buffs ) *
					( stats.GetStat( maximum_added_fire_damage_per_active_buff ) + stats.GetStat( maximum_added_fire_spell_damage_per_active_buff ) ) );
		}

		VIRTUAL_STAT( spell_total_minimum_added_cold_damage,
			global_total_minimum_added_cold_damage,
			spell_minimum_added_cold_damage,
			spell_and_attack_minimum_added_cold_damage )
		{
			return stats.GetStat( global_total_minimum_added_cold_damage ) + stats.GetStat( spell_minimum_added_cold_damage ) + stats.GetStat( spell_and_attack_minimum_added_cold_damage );
		}

		VIRTUAL_STAT( spell_total_maximum_added_cold_damage,
			global_total_maximum_added_cold_damage,
			spell_maximum_added_cold_damage,
			spell_and_attack_maximum_added_cold_damage )
		{
			return stats.GetStat( global_total_maximum_added_cold_damage ) + stats.GetStat( spell_maximum_added_cold_damage ) + stats.GetStat( spell_and_attack_maximum_added_cold_damage );
		}

		VIRTUAL_STAT( spell_total_minimum_added_lightning_damage,
			global_total_minimum_added_lightning_damage,
			spell_minimum_added_lightning_damage,
			spell_minimum_added_lightning_damage_while_unarmed, main_hand_weapon_type,
			spell_and_attack_minimum_added_lightning_damage )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_lightning_damage ) +
				stats.GetStat( spell_minimum_added_lightning_damage ) +
				stats.GetStat( spell_and_attack_minimum_added_lightning_damage ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( spell_minimum_added_lightning_damage_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( spell_total_maximum_added_lightning_damage,
			global_total_maximum_added_lightning_damage,
			spell_maximum_added_lightning_damage,
			spell_maximum_added_lightning_damage_while_unarmed, main_hand_weapon_type,
			spell_and_attack_maximum_added_lightning_damage )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_lightning_damage ) + stats.GetStat( spell_maximum_added_lightning_damage ) + stats.GetStat( spell_and_attack_maximum_added_lightning_damage ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( spell_maximum_added_lightning_damage_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( spell_total_minimum_added_chaos_damage,
			global_minimum_added_chaos_damage,
			spell_minimum_added_chaos_damage )
		{
			return stats.GetStat( global_minimum_added_chaos_damage ) + stats.GetStat( spell_minimum_added_chaos_damage );
		}

		VIRTUAL_STAT( spell_total_maximum_added_chaos_damage,
			global_maximum_added_chaos_damage,
			spell_maximum_added_chaos_damage )
		{
			return stats.GetStat( global_maximum_added_chaos_damage ) + stats.GetStat( spell_maximum_added_chaos_damage );
		}

		// Secondary Damage

		VIRTUAL_STAT( secondary_minimum_physical_damage,
			//base damage values
			secondary_minimum_base_physical_damage,
			global_total_minimum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_physical_damage )
		{
			if ( stats.GetStat( deal_no_secondary_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( secondary_minimum_base_physical_damage ) +
				( stats.GetStat( global_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( secondary_maximum_physical_damage,
			//base damage values
			secondary_maximum_base_physical_damage,
			global_total_maximum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_physical_damage )
		{
			if ( stats.GetStat( deal_no_secondary_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( secondary_maximum_base_physical_damage ) +
				( stats.GetStat( global_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( secondary_minimum_fire_damage,
			//base damage
			secondary_minimum_base_fire_damage,
			global_total_minimum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_fire_damage_pluspercent,
			combined_fire_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			secondary_minimum_base_physical_damage,
			global_total_minimum_added_physical_damage,
			secondary_minimum_base_cold_damage,
			global_total_minimum_added_cold_damage,
			secondary_minimum_base_lightning_damage,
			global_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			combined_cold_damage_pluspercent,
			combined_cold_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_fire_damage )
		{
			if ( stats.GetStat( deal_no_secondary_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( secondary_minimum_base_fire_damage ) +
				( stats.GetStat( global_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_minimum_base_physical_damage ) +
				( stats.GetStat( global_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( secondary_minimum_base_cold_damage ) +
				( stats.GetStat( global_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( secondary_minimum_base_lightning_damage ) +
				( stats.GetStat( global_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_maximum_fire_damage,
			//base damage
			secondary_maximum_base_fire_damage,
			global_total_maximum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_fire_damage_pluspercent,
			combined_fire_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			secondary_maximum_base_physical_damage,
			global_total_maximum_added_physical_damage,
			secondary_maximum_base_cold_damage,
			global_total_maximum_added_cold_damage,
			secondary_maximum_base_lightning_damage,
			global_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			combined_cold_damage_pluspercent,
			combined_cold_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_fire_damage )
		{
			if ( stats.GetStat( deal_no_secondary_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( secondary_maximum_base_fire_damage ) +
				( stats.GetStat( global_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_maximum_base_physical_damage ) +
				( stats.GetStat( global_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( secondary_maximum_base_cold_damage ) +
				( stats.GetStat( global_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( secondary_maximum_base_lightning_damage ) +
				( stats.GetStat( global_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_minimum_cold_damage,
			//base damage
			secondary_minimum_base_cold_damage,
			global_total_minimum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_cold_damage_pluspercent,
			combined_cold_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			secondary_minimum_base_physical_damage,
			global_total_minimum_added_physical_damage,
			secondary_minimum_base_lightning_damage,
			global_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_cold_damage )
		{
			if ( stats.GetStat( deal_no_secondary_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( secondary_minimum_base_cold_damage ) +
				( stats.GetStat( global_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_minimum_base_physical_damage ) +
				( stats.GetStat( global_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( secondary_minimum_base_lightning_damage ) +
				( stats.GetStat( global_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_cold_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_maximum_cold_damage,
			//base damage
			secondary_maximum_base_cold_damage,
			global_total_maximum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_cold_damage_pluspercent,
			combined_cold_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			secondary_maximum_base_physical_damage,
			global_total_maximum_added_physical_damage,
			secondary_maximum_base_lightning_damage,
			global_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_cold_damage )
		{
			if ( stats.GetStat( deal_no_secondary_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( secondary_maximum_base_cold_damage ) +
				( stats.GetStat( global_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_maximum_base_physical_damage ) +
				( stats.GetStat( global_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( secondary_maximum_base_lightning_damage ) +
				( stats.GetStat( global_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_cold_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_minimum_lightning_damage,
			//base damage
			secondary_minimum_base_lightning_damage,
			global_total_minimum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			secondary_minimum_base_physical_damage,
			global_total_minimum_added_physical_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_lightning_damage )
		{
			if ( stats.GetStat( deal_no_secondary_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( secondary_minimum_base_lightning_damage ) +
				( stats.GetStat( global_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_minimum_base_physical_damage ) +
				( stats.GetStat( global_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_lightning_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_maximum_lightning_damage,
			//base damage
			secondary_maximum_base_lightning_damage,
			global_total_maximum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			secondary_maximum_base_physical_damage,
			global_total_maximum_added_physical_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_lightning_damage )
		{
			if ( stats.GetStat( deal_no_secondary_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( secondary_maximum_base_lightning_damage ) +
				( stats.GetStat( global_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_maximum_base_physical_damage ) +
				( stats.GetStat( global_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_lightning_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_minimum_chaos_damage,
			//base damage
			secondary_minimum_base_chaos_damage,
			global_minimum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_chaos_damage_pluspercent,
			combined_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			secondary_minimum_base_physical_damage,
			global_total_minimum_added_physical_damage,
			secondary_minimum_base_fire_damage,
			global_total_minimum_added_fire_damage,
			secondary_minimum_base_cold_damage,
			global_total_minimum_added_cold_damage,
			secondary_minimum_base_lightning_damage,
			global_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			combined_fire_damage_pluspercent,
			combined_fire_damage_pluspercent_final,
			combined_cold_damage_pluspercent,
			combined_cold_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_chaos_damage )
		{
			if ( stats.GetStat( deal_no_secondary_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( secondary_minimum_base_chaos_damage ) +
				( stats.GetStat( global_minimum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_minimum_base_physical_damage ) +
				( stats.GetStat( global_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( secondary_minimum_base_fire_damage ) +
				( stats.GetStat( global_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( secondary_minimum_base_cold_damage ) +
				( stats.GetStat( global_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( secondary_minimum_base_lightning_damage ) +
				( stats.GetStat( global_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_cold_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_cold_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		VIRTUAL_STAT( secondary_maximum_chaos_damage,
			//base damage
			secondary_maximum_base_chaos_damage,
			global_maximum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_all_damage_pluspercent,
			combined_all_damage_pluspercent_final,
			combined_chaos_damage_pluspercent,
			combined_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			secondary_maximum_base_physical_damage,
			global_total_maximum_added_physical_damage,
			secondary_maximum_base_fire_damage,
			global_total_maximum_added_fire_damage,
			secondary_maximum_base_cold_damage,
			global_total_maximum_added_cold_damage,
			secondary_maximum_base_lightning_damage,
			global_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_physical_damage_pluspercent,
			combined_physical_damage_pluspercent_final,
			combined_fire_damage_pluspercent,
			combined_fire_damage_pluspercent_final,
			combined_cold_damage_pluspercent,
			combined_cold_damage_pluspercent_final,
			combined_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent_final,
			combined_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_secondary_chaos_damage )
		{
			if ( stats.GetStat( deal_no_secondary_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( secondary_maximum_base_chaos_damage ) +
				( stats.GetStat( global_maximum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( secondary_maximum_base_physical_damage ) +
				( stats.GetStat( global_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( secondary_maximum_base_fire_damage ) +
				( stats.GetStat( global_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( secondary_maximum_base_cold_damage ) +
				( stats.GetStat( global_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( secondary_maximum_base_lightning_damage ) +
				( stats.GetStat( global_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( combined_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_cold_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_lightning_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_cold_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( combined_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		//Main hand attack damage

		VIRTUAL_STAT( main_hand_minimum_physical_damage,
			//base damage values
			main_hand_total_minimum_base_physical_damage,
			main_hand_total_minimum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_physical_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( main_hand_maximum_physical_damage,
			//base damage values
			main_hand_total_maximum_base_physical_damage,
			main_hand_total_maximum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_physical_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( main_hand_minimum_fire_damage,
			//base damage
			main_hand_total_minimum_base_fire_damage,
			main_hand_total_minimum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_fire_damage_pluspercent,
			combined_main_hand_attack_fire_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			main_hand_total_minimum_base_physical_damage,
			main_hand_total_minimum_added_physical_damage,
			main_hand_total_minimum_base_cold_damage,
			main_hand_total_minimum_added_cold_damage,
			main_hand_total_minimum_base_lightning_damage,
			main_hand_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_main_hand_attack_cold_damage_pluspercent,
			combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_fire_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( main_hand_total_minimum_base_fire_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( main_hand_total_minimum_base_cold_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_maximum_fire_damage,
			//base damage
			main_hand_total_maximum_base_fire_damage,
			main_hand_total_maximum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_fire_damage_pluspercent,
			combined_main_hand_attack_fire_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			main_hand_total_maximum_base_physical_damage,
			main_hand_total_maximum_added_physical_damage,
			main_hand_total_maximum_base_cold_damage,
			main_hand_total_maximum_added_cold_damage,
			main_hand_total_maximum_base_lightning_damage,
			main_hand_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_main_hand_attack_cold_damage_pluspercent,
			combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_fire_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( main_hand_total_maximum_base_fire_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( main_hand_total_maximum_base_cold_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_minimum_cold_damage,
			//base damage
			main_hand_total_minimum_base_cold_damage,
			main_hand_total_minimum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_cold_damage_pluspercent,
			combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			main_hand_total_minimum_base_physical_damage,
			main_hand_total_minimum_added_physical_damage,
			main_hand_total_minimum_base_lightning_damage,
			main_hand_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_cold_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( main_hand_total_minimum_base_cold_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_maximum_cold_damage,
			//base damage
			main_hand_total_maximum_base_cold_damage,
			main_hand_total_maximum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_cold_damage_pluspercent,
			combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			main_hand_total_maximum_base_physical_damage,
			main_hand_total_maximum_added_physical_damage,
			main_hand_total_maximum_base_lightning_damage,
			main_hand_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_cold_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( main_hand_total_maximum_base_cold_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_minimum_lightning_damage,
			//base damage
			main_hand_total_minimum_base_lightning_damage,
			main_hand_total_minimum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			main_hand_total_minimum_base_physical_damage,
			main_hand_total_minimum_added_physical_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_lightning_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_maximum_lightning_damage,
			//base damage
			main_hand_total_maximum_base_lightning_damage,
			main_hand_total_maximum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			main_hand_total_maximum_base_physical_damage,
			main_hand_total_maximum_added_physical_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_lightning_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_minimum_chaos_damage,
			//base damage
			main_hand_total_minimum_base_chaos_damage,
			main_hand_total_minimum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_chaos_damage_pluspercent,
			combined_main_hand_attack_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			main_hand_total_minimum_base_physical_damage,
			main_hand_total_minimum_added_physical_damage,
			main_hand_total_minimum_base_fire_damage,
			main_hand_total_minimum_added_fire_damage,
			main_hand_total_minimum_base_cold_damage,
			main_hand_total_minimum_added_cold_damage,
			main_hand_total_minimum_base_lightning_damage,
			main_hand_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_main_hand_attack_fire_damage_pluspercent,
			combined_main_hand_attack_fire_damage_pluspercent_final,
			combined_main_hand_attack_cold_damage_pluspercent,
			combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_chaos_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( main_hand_total_minimum_base_chaos_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( main_hand_total_minimum_base_fire_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( main_hand_total_minimum_base_cold_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		VIRTUAL_STAT( main_hand_maximum_chaos_damage,
			//base damage
			main_hand_total_maximum_base_chaos_damage,
			main_hand_total_maximum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_main_hand_attack_all_damage_pluspercent,
			combined_main_hand_attack_all_damage_pluspercent_final,
			combined_main_hand_attack_chaos_damage_pluspercent,
			combined_main_hand_attack_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			main_hand_total_maximum_base_physical_damage,
			main_hand_total_maximum_added_physical_damage,
			main_hand_total_maximum_base_fire_damage,
			main_hand_total_maximum_added_fire_damage,
			main_hand_total_maximum_base_cold_damage,
			main_hand_total_maximum_added_cold_damage,
			main_hand_total_maximum_base_lightning_damage,
			main_hand_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_main_hand_attack_physical_damage_pluspercent,
			combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_main_hand_attack_fire_damage_pluspercent,
			combined_main_hand_attack_fire_damage_pluspercent_final,
			combined_main_hand_attack_cold_damage_pluspercent,
			combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_main_hand_attack_lightning_damage_pluspercent,
			combined_main_hand_attack_lightning_damage_pluspercent_final,
			combined_main_hand_attack_elemental_damage_pluspercent,
			combined_main_hand_attack_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_main_hand_chaos_damage )
		{
			if ( stats.GetStat( deal_no_main_hand_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( main_hand_total_maximum_base_chaos_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( main_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( main_hand_total_maximum_base_fire_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( main_hand_total_maximum_base_cold_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( main_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( main_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_main_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_main_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_main_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_main_hand_attack_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		//Off hand attack damage

		VIRTUAL_STAT( off_hand_minimum_physical_damage,
			//base damage values
			off_hand_total_minimum_base_physical_damage,
			off_hand_total_minimum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_physical_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( off_hand_maximum_physical_damage,
			//base damage values
			off_hand_total_maximum_base_physical_damage,
			off_hand_total_maximum_added_physical_damage,
			//damage converted away
			physical_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_physical_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_physical_damage ) )
				return 0;

			//base damage
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//physical damage
			const auto base_physical_damage = unmodified_physical_damage * Scale( 100 - stats.GetStat( physical_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round( base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale ) );
		}

		VIRTUAL_STAT( off_hand_minimum_fire_damage,
			//base damage
			off_hand_total_minimum_base_fire_damage,
			off_hand_total_minimum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_fire_damage_pluspercent,
			combined_off_hand_attack_fire_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			off_hand_total_minimum_base_physical_damage,
			off_hand_total_minimum_added_physical_damage,
			off_hand_total_minimum_base_cold_damage,
			off_hand_total_minimum_added_cold_damage,
			off_hand_total_minimum_base_lightning_damage,
			off_hand_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_off_hand_attack_cold_damage_pluspercent,
			combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_fire_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( off_hand_total_minimum_base_fire_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( off_hand_total_minimum_base_cold_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_maximum_fire_damage,
			//base damage
			off_hand_total_maximum_base_fire_damage,
			off_hand_total_maximum_added_fire_damage,
			//damage converted away
			fire_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_fire_damage_pluspercent,
			combined_off_hand_attack_fire_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			total_lightning_damage_percent_as_fire,
			//base damage converted to
			off_hand_total_maximum_base_physical_damage,
			off_hand_total_maximum_added_physical_damage,
			off_hand_total_maximum_base_cold_damage,
			off_hand_total_maximum_added_cold_damage,
			off_hand_total_maximum_base_lightning_damage,
			off_hand_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_off_hand_attack_cold_damage_pluspercent,
			combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_fire_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_fire_damage ) )
				return 0;

			//base damage
			const auto unmodified_fire_damage = stats.GetStat( off_hand_total_maximum_base_fire_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( off_hand_total_maximum_base_cold_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//fire damage
			const auto base_fire_damage = unmodified_fire_damage * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = fire_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = fire_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( 100 - stats.GetStat( fire_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = fire_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto cold_damage_scale = fire_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_minimum_cold_damage,
			//base damage
			off_hand_total_minimum_base_cold_damage,
			off_hand_total_minimum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_cold_damage_pluspercent,
			combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			off_hand_total_minimum_base_physical_damage,
			off_hand_total_minimum_added_physical_damage,
			off_hand_total_minimum_base_lightning_damage,
			off_hand_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_cold_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( off_hand_total_minimum_base_cold_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_maximum_cold_damage,
			//base damage
			off_hand_total_maximum_base_cold_damage,
			off_hand_total_maximum_added_cold_damage,
			//damage converted away
			cold_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_cold_damage_pluspercent,
			combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_cold,
			total_lightning_damage_percent_as_cold,
			//base damage converted to
			off_hand_total_maximum_base_physical_damage,
			off_hand_total_maximum_added_physical_damage,
			off_hand_total_maximum_base_lightning_damage,
			off_hand_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_lightning,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_cold_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_cold_damage ) )
				return 0;

			//base damage
			const auto unmodified_cold_damage = stats.GetStat( off_hand_total_maximum_base_cold_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//cold damage
			const auto base_cold_damage = unmodified_cold_damage * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = cold_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( 100 - stats.GetStat( cold_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = cold_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto lightning_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_minimum_lightning_damage,
			//base damage
			off_hand_total_minimum_base_lightning_damage,
			off_hand_total_minimum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			off_hand_total_minimum_base_physical_damage,
			off_hand_total_minimum_added_physical_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_lightning_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_maximum_lightning_damage,
			//base damage
			off_hand_total_maximum_base_lightning_damage,
			off_hand_total_maximum_added_lightning_damage,
			//damage converted away
			lightning_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_lightning,
			//base damage converted to
			off_hand_total_maximum_base_physical_damage,
			off_hand_total_maximum_added_physical_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_lightning_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_lightning_damage ) )
				return 0;

			//base damage
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//lightning damage
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( 100 - stats.GetStat( lightning_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_minimum_chaos_damage,
			//base damage
			off_hand_total_minimum_base_chaos_damage,
			off_hand_total_minimum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_chaos_damage_pluspercent,
			combined_off_hand_attack_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			off_hand_total_minimum_base_physical_damage,
			off_hand_total_minimum_added_physical_damage,
			off_hand_total_minimum_base_fire_damage,
			off_hand_total_minimum_added_fire_damage,
			off_hand_total_minimum_base_cold_damage,
			off_hand_total_minimum_added_cold_damage,
			off_hand_total_minimum_base_lightning_damage,
			off_hand_total_minimum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_off_hand_attack_fire_damage_pluspercent,
			combined_off_hand_attack_fire_damage_pluspercent_final,
			combined_off_hand_attack_cold_damage_pluspercent,
			combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_chaos_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( off_hand_total_minimum_base_chaos_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_minimum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( off_hand_total_minimum_base_fire_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( off_hand_total_minimum_base_cold_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_minimum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_minimum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		VIRTUAL_STAT( off_hand_maximum_chaos_damage,
			//base damage
			off_hand_total_maximum_base_chaos_damage,
			off_hand_total_maximum_added_chaos_damage,
			//damage converted away
			chaos_damage_percent_lost_to_conversion,
			//damage modifiers
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			combined_off_hand_attack_all_damage_pluspercent,
			combined_off_hand_attack_all_damage_pluspercent_final,
			combined_off_hand_attack_chaos_damage_pluspercent,
			combined_off_hand_attack_chaos_damage_pluspercent_final,
			//damage converted to
			total_physical_damage_percent_as_chaos,
			total_fire_damage_percent_as_chaos,
			total_cold_damage_percent_as_chaos,
			total_lightning_damage_percent_as_chaos,
			//base damage converted to
			off_hand_total_maximum_base_physical_damage,
			off_hand_total_maximum_added_physical_damage,
			off_hand_total_maximum_base_fire_damage,
			off_hand_total_maximum_added_fire_damage,
			off_hand_total_maximum_base_cold_damage,
			off_hand_total_maximum_added_cold_damage,
			off_hand_total_maximum_base_lightning_damage,
			off_hand_total_maximum_added_lightning_damage,
			//other modifiers for damage converted to
			combined_off_hand_attack_physical_damage_pluspercent,
			combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_off_hand_attack_fire_damage_pluspercent,
			combined_off_hand_attack_fire_damage_pluspercent_final,
			combined_off_hand_attack_cold_damage_pluspercent,
			combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_off_hand_attack_lightning_damage_pluspercent,
			combined_off_hand_attack_lightning_damage_pluspercent_final,
			combined_off_hand_attack_elemental_damage_pluspercent,
			combined_off_hand_attack_elemental_damage_pluspercent_final,
			//in-between conversions
			total_physical_damage_percent_as_fire,
			total_physical_damage_percent_as_cold,
			total_physical_damage_percent_as_lightning,
			total_lightning_damage_percent_as_cold,
			total_lightning_damage_percent_as_fire,
			total_cold_damage_percent_as_fire,
			minion_added_damage_pluspercent_final_from_skill,
			deal_no_off_hand_chaos_damage )
		{
			if ( stats.GetStat( deal_no_off_hand_chaos_damage ) )
				return 0;

			//base damage
			const auto unmodified_chaos_damage = stats.GetStat( off_hand_total_maximum_base_chaos_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_chaos_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			//other base damages
			const auto unmodified_physical_damage = stats.GetStat( off_hand_total_maximum_base_physical_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_physical_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_fire_damage = stats.GetStat( off_hand_total_maximum_base_fire_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_fire_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_cold_damage = stats.GetStat( off_hand_total_maximum_base_cold_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_cold_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );
			const auto unmodified_lightning_damage = stats.GetStat( off_hand_total_maximum_base_lightning_damage ) +
				( stats.GetStat( off_hand_total_maximum_added_lightning_damage ) * Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) )
					* Scale( 100 + stats.GetStat( minion_added_damage_pluspercent_final_from_skill ) ) );

			//chaos damage
			const auto base_chaos_damage = unmodified_chaos_damage * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto chaos_damage_increase = stats.GetStat( combined_off_hand_attack_all_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_chaos_damage_pluspercent );
			const auto chaos_damage_scale = Scale( 100 + stats.GetStat( combined_off_hand_attack_all_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_chaos_damage_pluspercent_final ) );
			//converted from physical
			const auto base_physical_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent );
			const auto physical_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_physical_damage_pluspercent_final ) );
			//converted from physical via lightning
			const auto base_physical_lightning_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto physical_lightning_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via cold
			const auto base_physical_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto physical_cold_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via fire
			const auto base_physical_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_fire_damage_increase = physical_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto physical_fire_damage_scale = physical_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from physical via lightning and cold
			const auto base_physical_lightning_cold_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent );
			const auto physical_lightning_cold_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) );
			//converted from physical via lightning and fire
			const auto base_physical_lightning_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_fire_damage_increase = physical_lightning_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_fire_damage_scale = physical_lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via cold and fire
			const auto base_physical_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_cold_fire_damage_increase = physical_cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto physical_cold_fire_damage_scale = physical_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from physical via lightning, cold and fire
			const auto base_physical_lightning_cold_fire_damage = unmodified_physical_damage * Scale( stats.GetStat( total_physical_damage_percent_as_lightning ) ) * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto physical_lightning_cold_fire_damage_increase = physical_lightning_cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto physical_lightning_cold_fire_damage_scale = physical_lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning
			const auto base_lightning_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto lightning_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_lightning_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from lightning via cold
			const auto base_lightning_cold_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent );
			const auto lightning_cold_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) );
			//converted from lightning via fire
			const auto base_lightning_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_fire_damage_increase = lightning_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto lightning_fire_damage_scale = lightning_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from lightning via cold and fire
			const auto base_lightning_cold_fire_damage = unmodified_lightning_damage * Scale( stats.GetStat( total_lightning_damage_percent_as_cold ) ) * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto lightning_cold_fire_damage_increase = lightning_cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto lightning_cold_fire_damage_scale = lightning_cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from cold
			const auto base_cold_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto cold_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_cold_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );
			//converted from cold via fire
			const auto base_cold_fire_damage = unmodified_cold_damage * Scale( stats.GetStat( total_cold_damage_percent_as_fire ) ) * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto cold_fire_damage_increase = cold_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent );
			const auto cold_fire_damage_scale = cold_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) );
			//converted from fire
			const auto base_fire_damage = unmodified_fire_damage * Scale( stats.GetStat( total_fire_damage_percent_as_chaos ) ) * Scale( 100 - stats.GetStat( chaos_damage_percent_lost_to_conversion ) );
			const auto fire_damage_increase = chaos_damage_increase + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent ) + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent );
			const auto fire_damage_scale = chaos_damage_scale * Scale( 100 + stats.GetStat( combined_off_hand_attack_fire_damage_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_off_hand_attack_elemental_damage_pluspercent_final ) );

			//total
			return std::max( 0, Round(
				base_chaos_damage * Scale( 100 + chaos_damage_increase ) * chaos_damage_scale
				+ base_physical_damage * Scale( 100 + physical_damage_increase ) * physical_damage_scale
				+ base_physical_lightning_damage * Scale( 100 + physical_lightning_damage_increase ) * physical_lightning_damage_scale
				+ base_physical_cold_damage * Scale( 100 + physical_cold_damage_increase ) * physical_cold_damage_scale
				+ base_physical_fire_damage * Scale( 100 + physical_fire_damage_increase ) * physical_fire_damage_scale
				+ base_physical_lightning_cold_damage * Scale( 100 + physical_lightning_cold_damage_increase ) * physical_lightning_cold_damage_scale
				+ base_physical_lightning_fire_damage * Scale( 100 + physical_lightning_fire_damage_increase ) * physical_lightning_fire_damage_scale
				+ base_physical_cold_fire_damage * Scale( 100 + physical_cold_fire_damage_increase ) * physical_cold_fire_damage_scale
				+ base_physical_lightning_cold_fire_damage * Scale( 100 + physical_lightning_cold_fire_damage_increase ) * physical_lightning_cold_fire_damage_scale
				+ base_lightning_damage * Scale( 100 + lightning_damage_increase ) * lightning_damage_scale
				+ base_lightning_cold_damage * Scale( 100 + lightning_cold_damage_increase ) * lightning_cold_damage_scale
				+ base_lightning_fire_damage * Scale( 100 + lightning_fire_damage_increase ) * lightning_fire_damage_scale
				+ base_lightning_cold_fire_damage * Scale( 100 + lightning_cold_fire_damage_increase ) * lightning_cold_fire_damage_scale
				+ base_cold_damage * Scale( 100 + cold_damage_increase ) * cold_damage_scale
				+ base_cold_fire_damage * Scale( 100 + cold_fire_damage_increase ) * cold_fire_damage_scale
				+ base_fire_damage * Scale( 100 + fire_damage_increase ) * fire_damage_scale
				) );
		}

		//Attack Base Damage

		VIRTUAL_STAT( main_hand_total_minimum_base_physical_damage,
			main_hand_local_physical_damage_pluspercent,
			main_hand_quality,
			main_hand_local_minimum_added_physical_damage )
		{
			return Round( stats.GetStat( main_hand_local_minimum_added_physical_damage ) *
				Scale( 100 + stats.GetStat( main_hand_local_physical_damage_pluspercent ) +
					stats.GetStat( main_hand_quality ) ) );
		}

		VIRTUAL_STAT( main_hand_total_maximum_base_physical_damage,
			main_hand_local_physical_damage_pluspercent,
			main_hand_quality,
			main_hand_local_maximum_added_physical_damage )
		{
			return Round( stats.GetStat( main_hand_local_maximum_added_physical_damage ) *
				Scale( 100 + stats.GetStat( main_hand_local_physical_damage_pluspercent ) +
					stats.GetStat( main_hand_quality ) ) );
		}

		VIRTUAL_STAT( main_hand_total_minimum_base_fire_damage,
			main_hand_local_minimum_added_fire_damage )
		{
			return stats.GetStat( main_hand_local_minimum_added_fire_damage );
		}

		VIRTUAL_STAT( main_hand_total_maximum_base_fire_damage,
			main_hand_local_maximum_added_fire_damage )
		{
			return stats.GetStat( main_hand_local_maximum_added_fire_damage );
		}

		VIRTUAL_STAT( main_hand_total_minimum_base_cold_damage,
			main_hand_local_minimum_added_cold_damage )
		{
			return stats.GetStat( main_hand_local_minimum_added_cold_damage );
		}

		VIRTUAL_STAT( main_hand_total_maximum_base_cold_damage,
			main_hand_local_maximum_added_cold_damage )
		{
			return stats.GetStat( main_hand_local_maximum_added_cold_damage );
		}

		VIRTUAL_STAT( main_hand_total_minimum_base_lightning_damage,
			main_hand_local_minimum_added_lightning_damage )
		{
			return stats.GetStat( main_hand_local_minimum_added_lightning_damage );
		}

		VIRTUAL_STAT( main_hand_total_maximum_base_lightning_damage,
			main_hand_local_maximum_added_lightning_damage )
		{
			return stats.GetStat( main_hand_local_maximum_added_lightning_damage );
		}

		VIRTUAL_STAT( main_hand_total_minimum_base_chaos_damage,
			main_hand_local_minimum_added_chaos_damage )
		{
			return stats.GetStat( main_hand_local_minimum_added_chaos_damage );
		}

		VIRTUAL_STAT( main_hand_total_maximum_base_chaos_damage,
			main_hand_local_maximum_added_chaos_damage )
		{
			return stats.GetStat( main_hand_local_maximum_added_chaos_damage );
		}

		VIRTUAL_STAT( off_hand_total_minimum_base_physical_damage,
			off_hand_local_physical_damage_pluspercent,
			off_hand_quality,
			off_hand_local_minimum_added_physical_damage )
		{
			return Round( stats.GetStat( off_hand_local_minimum_added_physical_damage ) *
				Scale( 100 + stats.GetStat( off_hand_local_physical_damage_pluspercent ) +
					stats.GetStat( off_hand_quality ) ) );
		}

		VIRTUAL_STAT( off_hand_total_maximum_base_physical_damage,
			off_hand_local_physical_damage_pluspercent,
			off_hand_quality,
			off_hand_local_maximum_added_physical_damage )
		{
			return Round( stats.GetStat( off_hand_local_maximum_added_physical_damage ) *
				Scale( 100 + stats.GetStat( off_hand_local_physical_damage_pluspercent ) +
					stats.GetStat( off_hand_quality ) ) );
		}

		VIRTUAL_STAT( off_hand_total_minimum_base_fire_damage,
			off_hand_local_minimum_added_fire_damage )
		{
			return stats.GetStat( off_hand_local_minimum_added_fire_damage );
		}

		VIRTUAL_STAT( off_hand_total_maximum_base_fire_damage,
			off_hand_local_maximum_added_fire_damage )
		{
			return stats.GetStat( off_hand_local_maximum_added_fire_damage );
		}

		VIRTUAL_STAT( off_hand_total_minimum_base_cold_damage,
			off_hand_local_minimum_added_cold_damage )
		{
			return stats.GetStat( off_hand_local_minimum_added_cold_damage );
		}

		VIRTUAL_STAT( off_hand_total_maximum_base_cold_damage,
			off_hand_local_maximum_added_cold_damage )
		{
			return stats.GetStat( off_hand_local_maximum_added_cold_damage );
		}

		VIRTUAL_STAT( off_hand_total_minimum_base_lightning_damage,
			off_hand_local_minimum_added_lightning_damage )
		{
			return stats.GetStat( off_hand_local_minimum_added_lightning_damage );
		}

		VIRTUAL_STAT( off_hand_total_maximum_base_lightning_damage,
			off_hand_local_maximum_added_lightning_damage )
		{
			return stats.GetStat( off_hand_local_maximum_added_lightning_damage );
		}

		VIRTUAL_STAT( off_hand_total_minimum_base_chaos_damage,
			off_hand_local_minimum_added_chaos_damage )
		{
			return stats.GetStat( off_hand_local_minimum_added_chaos_damage );
		}

		VIRTUAL_STAT( off_hand_total_maximum_base_chaos_damage,
			off_hand_local_maximum_added_chaos_damage )
		{
			return stats.GetStat( off_hand_local_maximum_added_chaos_damage );
		}

		//Attack added damage

		VIRTUAL_STAT( main_hand_total_minimum_added_physical_damage,
			global_total_minimum_added_physical_damage,
			attack_minimum_added_physical_damage,
			minion_attack_minimum_added_physical_damage, modifiers_to_minion_damage_also_affect_you,
			attack_minimum_added_physical_damage_with_bow, main_hand_weapon_type,
			attack_minimum_added_physical_damage_per_25_dexterity, dexterity )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_physical_damage ) + stats.GetStat( attack_minimum_added_physical_damage ) +
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? stats.GetStat( minion_attack_minimum_added_physical_damage ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( attack_minimum_added_physical_damage_with_bow ) : 0 ) +
				( stats.GetStat( attack_minimum_added_physical_damage_per_25_dexterity ) * ( stats.GetStat( dexterity ) / 25 ) );
		}

		VIRTUAL_STAT( main_hand_total_maximum_added_physical_damage,
			global_total_maximum_added_physical_damage,
			attack_maximum_added_physical_damage,
			minion_attack_maximum_added_physical_damage, modifiers_to_minion_damage_also_affect_you,
			attack_maximum_added_physical_damage_with_bow, main_hand_weapon_type,
			attack_maximum_added_physical_damage_per_25_dexterity, dexterity )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_physical_damage ) + stats.GetStat( attack_maximum_added_physical_damage ) +
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? stats.GetStat( minion_attack_maximum_added_physical_damage ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( attack_maximum_added_physical_damage_with_bow ) : 0 ) +
				( stats.GetStat( attack_maximum_added_physical_damage_per_25_dexterity ) * ( stats.GetStat( dexterity ) / 25 ) );
		}

		VIRTUAL_STAT( main_hand_total_minimum_added_fire_damage,
			global_total_minimum_added_fire_damage,
			attack_minimum_added_fire_damage,
			attack_minimum_added_fire_damage_with_wand, main_hand_weapon_type,
			attack_minimum_added_fire_damage_with_bow,
			number_of_active_buffs, minimum_added_fire_damage_per_active_buff,
			minimum_added_fire_attack_damage_per_active_buff )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_fire_damage ) + stats.GetStat( attack_minimum_added_fire_damage ) +
				( main_hand_weapon_index == Items::Wand ? stats.GetStat( attack_minimum_added_fire_damage_with_wand ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( attack_minimum_added_fire_damage_with_bow ) : 0 ) +
				( stats.GetStat( number_of_active_buffs ) *
					( stats.GetStat( minimum_added_fire_damage_per_active_buff ) + stats.GetStat( minimum_added_fire_attack_damage_per_active_buff ) ) );
		}

		VIRTUAL_STAT( main_hand_total_maximum_added_fire_damage,
			global_total_maximum_added_fire_damage,
			attack_maximum_added_fire_damage,
			attack_maximum_added_fire_damage_with_wand, main_hand_weapon_type,
			attack_maximum_added_fire_damage_with_bow,
			number_of_active_buffs, 
			minimum_added_fire_damage_per_active_buff,
			maximum_added_fire_damage_per_active_buff,
			minimum_added_fire_attack_damage_per_active_buff,
			maximum_added_fire_attack_damage_per_active_buff )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_fire_damage ) + stats.GetStat( attack_maximum_added_fire_damage ) +
				( main_hand_weapon_index == Items::Wand ? stats.GetStat( attack_maximum_added_fire_damage_with_wand ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( attack_maximum_added_fire_damage_with_bow ) : 0 ) +
				( stats.GetStat( number_of_active_buffs ) *
					( stats.GetStat( minimum_added_fire_attack_damage_per_active_buff ) + stats.GetStat( maximum_added_fire_attack_damage_per_active_buff ) +
						stats.GetStat( minimum_added_fire_damage_per_active_buff ) + stats.GetStat( maximum_added_fire_damage_per_active_buff )) );
		}

		VIRTUAL_STAT( main_hand_total_minimum_added_cold_damage,
			global_total_minimum_added_cold_damage,
			attack_minimum_added_cold_damage,
			attack_minimum_added_cold_damage_with_wand, main_hand_weapon_type,
			spell_and_attack_minimum_added_cold_damage )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_cold_damage ) + stats.GetStat( attack_minimum_added_cold_damage ) + stats.GetStat( spell_and_attack_minimum_added_cold_damage ) +
				( main_hand_weapon_index == Items::Wand ? stats.GetStat( attack_minimum_added_cold_damage_with_wand ) : 0 );
		}

		VIRTUAL_STAT( main_hand_total_maximum_added_cold_damage,
			global_total_maximum_added_cold_damage,
			attack_maximum_added_cold_damage,
			attack_maximum_added_cold_damage_with_wand, main_hand_weapon_type,
			spell_and_attack_maximum_added_cold_damage )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_cold_damage ) + stats.GetStat( attack_maximum_added_cold_damage ) + stats.GetStat( spell_and_attack_maximum_added_cold_damage ) +
				( main_hand_weapon_index == Items::Wand ? stats.GetStat( attack_maximum_added_cold_damage_with_wand ) : 0 );
		}

		VIRTUAL_STAT( main_hand_total_minimum_added_lightning_damage,
			global_total_minimum_added_lightning_damage,
			attack_minimum_added_lightning_damage,
			attack_minimum_added_lightning_damage_with_wand, main_hand_weapon_type,
			attack_minimum_added_lightning_damage_while_unarmed,
			spell_and_attack_minimum_added_lightning_damage )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_lightning_damage ) + stats.GetStat( attack_minimum_added_lightning_damage ) + stats.GetStat( spell_and_attack_minimum_added_lightning_damage ) +
				( main_hand_weapon_index == Items::Wand ? stats.GetStat( attack_minimum_added_lightning_damage_with_wand ) : 0 ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( attack_minimum_added_lightning_damage_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( main_hand_total_maximum_added_lightning_damage,
			global_total_maximum_added_lightning_damage,
			attack_maximum_added_lightning_damage,
			attack_maximum_added_lightning_damage_with_wand, main_hand_weapon_type,
			attack_maximum_added_lightning_damage_while_unarmed,
			spell_and_attack_maximum_added_lightning_damage )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_lightning_damage ) + stats.GetStat( attack_maximum_added_lightning_damage ) + stats.GetStat( spell_and_attack_maximum_added_lightning_damage ) +
				( main_hand_weapon_index == Items::Wand ? stats.GetStat( attack_maximum_added_lightning_damage_with_wand ) : 0 ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( attack_maximum_added_lightning_damage_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( main_hand_total_minimum_added_chaos_damage,
			global_minimum_added_chaos_damage,
			attack_minimum_added_chaos_damage )
		{
			return stats.GetStat( global_minimum_added_chaos_damage ) + stats.GetStat( attack_minimum_added_chaos_damage );
		}

		VIRTUAL_STAT( main_hand_total_maximum_added_chaos_damage,
			global_maximum_added_chaos_damage,
			attack_maximum_added_chaos_damage )
		{
			return stats.GetStat( global_maximum_added_chaos_damage ) + stats.GetStat( attack_maximum_added_chaos_damage );
		}

		VIRTUAL_STAT( off_hand_total_minimum_added_physical_damage,
			global_total_minimum_added_physical_damage,
			attack_minimum_added_physical_damage,
			minion_attack_minimum_added_physical_damage, modifiers_to_minion_damage_also_affect_you,
			attack_minimum_added_physical_damage_per_25_dexterity, dexterity )
		{
			return stats.GetStat( global_total_minimum_added_physical_damage ) + stats.GetStat( attack_minimum_added_physical_damage ) +
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? stats.GetStat( minion_attack_minimum_added_physical_damage ) : 0 ) +
				( stats.GetStat( attack_minimum_added_physical_damage_per_25_dexterity ) * ( stats.GetStat( dexterity ) / 25 ) );
		}

		VIRTUAL_STAT( off_hand_total_maximum_added_physical_damage,
			global_total_maximum_added_physical_damage,
			attack_maximum_added_physical_damage,
			minion_attack_maximum_added_physical_damage, modifiers_to_minion_damage_also_affect_you,
			attack_maximum_added_physical_damage_per_25_dexterity, dexterity )
		{
			return stats.GetStat( global_total_maximum_added_physical_damage ) + stats.GetStat( attack_maximum_added_physical_damage ) +
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? stats.GetStat( minion_attack_maximum_added_physical_damage ) : 0 ) +
				( stats.GetStat( attack_maximum_added_physical_damage_per_25_dexterity ) * ( stats.GetStat( dexterity ) / 25 ) );
		}

		VIRTUAL_STAT( off_hand_total_minimum_added_fire_damage,
			global_total_minimum_added_fire_damage,
			attack_minimum_added_fire_damage,
			attack_minimum_added_fire_damage_with_wand, off_hand_weapon_type,
			number_of_active_buffs, minimum_added_fire_damage_per_active_buff,
			minimum_added_fire_attack_damage_per_active_buff )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_fire_damage ) + stats.GetStat( attack_minimum_added_fire_damage ) +
				( off_hand_weapon_index == Items::Wand ? stats.GetStat( attack_minimum_added_fire_damage_with_wand ) : 0 ) +
				( stats.GetStat( number_of_active_buffs ) *
					( stats.GetStat( minimum_added_fire_damage_per_active_buff ) + stats.GetStat( minimum_added_fire_attack_damage_per_active_buff ) ) );
		}

		VIRTUAL_STAT( off_hand_total_maximum_added_fire_damage,
			global_total_maximum_added_fire_damage,
			attack_maximum_added_fire_damage,
			attack_maximum_added_fire_damage_with_wand, off_hand_weapon_type,
			number_of_active_buffs, 
			minimum_added_fire_damage_per_active_buff,
			maximum_added_fire_damage_per_active_buff,
			minimum_added_fire_attack_damage_per_active_buff,
			maximum_added_fire_attack_damage_per_active_buff )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_fire_damage ) + stats.GetStat( attack_maximum_added_fire_damage ) +
				( off_hand_weapon_index == Items::Wand ? stats.GetStat( attack_maximum_added_fire_damage_with_wand ) : 0 ) +
				( stats.GetStat( number_of_active_buffs ) *
					( stats.GetStat( minimum_added_fire_damage_per_active_buff ) + stats.GetStat( maximum_added_fire_damage_per_active_buff ) +
						stats.GetStat( minimum_added_fire_attack_damage_per_active_buff ) + stats.GetStat( maximum_added_fire_attack_damage_per_active_buff )) );
		}

		VIRTUAL_STAT( off_hand_total_minimum_added_cold_damage,
			global_total_minimum_added_cold_damage,
			attack_minimum_added_cold_damage,
			attack_minimum_added_cold_damage_with_wand, off_hand_weapon_type,
			spell_and_attack_minimum_added_cold_damage )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_cold_damage ) + stats.GetStat( attack_minimum_added_cold_damage ) + stats.GetStat( spell_and_attack_minimum_added_cold_damage ) +
				( off_hand_weapon_index == Items::Wand ? stats.GetStat( attack_minimum_added_cold_damage_with_wand ) : 0 );
		}

		VIRTUAL_STAT( off_hand_total_maximum_added_cold_damage,
			global_total_maximum_added_cold_damage,
			attack_maximum_added_cold_damage,
			attack_maximum_added_cold_damage_with_wand, off_hand_weapon_type,
			spell_and_attack_maximum_added_cold_damage )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_cold_damage ) + stats.GetStat( attack_maximum_added_cold_damage ) + stats.GetStat( spell_and_attack_maximum_added_cold_damage ) +
				( off_hand_weapon_index == Items::Wand ? stats.GetStat( attack_maximum_added_cold_damage_with_wand ) : 0 );
		}

		VIRTUAL_STAT( off_hand_total_minimum_added_lightning_damage,
			global_total_minimum_added_lightning_damage,
			attack_minimum_added_lightning_damage,
			attack_minimum_added_lightning_damage_with_wand, off_hand_weapon_type,
			attack_minimum_added_lightning_damage_while_unarmed, main_hand_weapon_type )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( global_total_minimum_added_lightning_damage ) + stats.GetStat( attack_minimum_added_lightning_damage ) +
				( off_hand_weapon_index == Items::Wand ? stats.GetStat( attack_minimum_added_lightning_damage_with_wand ) : 0 ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( attack_minimum_added_lightning_damage_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( off_hand_total_maximum_added_lightning_damage,
			global_total_maximum_added_lightning_damage,
			attack_maximum_added_lightning_damage,
			attack_maximum_added_lightning_damage_with_wand, off_hand_weapon_type,
			attack_maximum_added_lightning_damage_while_unarmed, main_hand_weapon_type )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( global_total_maximum_added_lightning_damage ) + stats.GetStat( attack_maximum_added_lightning_damage ) +
				( off_hand_weapon_index == Items::Wand ? stats.GetStat( attack_maximum_added_lightning_damage_with_wand ) : 0 ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( attack_maximum_added_lightning_damage_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( off_hand_total_minimum_added_chaos_damage,
			global_minimum_added_chaos_damage,
			attack_minimum_added_chaos_damage )
		{
			return stats.GetStat( global_minimum_added_chaos_damage ) + stats.GetStat( attack_minimum_added_chaos_damage );
		}

		VIRTUAL_STAT( off_hand_total_maximum_added_chaos_damage,
			global_maximum_added_chaos_damage,
			attack_maximum_added_chaos_damage )
		{
			return stats.GetStat( global_maximum_added_chaos_damage ) + stats.GetStat( attack_maximum_added_chaos_damage );
		}

		///
		/// Combined damage increase stats
		///

		//all damage types
		VIRTUAL_STAT( combined_all_damage_pluspercent,
			damage_pluspercent,
			backstab_damage_pluspercent,
			projectile_damage_pluspercent, is_projectile,
			projectile_damage_pluspercent_per_power_charge, current_power_charges,
			arrow_pierce_chance_applies_to_projectile_damage, arrow_pierce_percent,
			damage_pluspercent_when_on_low_life, on_low_life,
			damage_pluspercent_when_on_full_life, on_full_life,
			area_damage_pluspercent, is_area_damage,
			damage_pluspercent_per_frenzy_charge, current_frenzy_charges,
			damage_while_no_frenzy_charges_pluspercent,
			damage_pluspercent_per_endurance_charge, current_endurance_charges,
			totem_damage_pluspercent, skill_is_totemified,
			trap_damage_pluspercent, skill_is_trapped,
			mine_damage_pluspercent, skill_is_mined,
			damage_pluspercent_per_shock, is_shocked,
			damage_plus1percent_per_X_strength, strength,
			damage_pluspercent_while_ignited, is_ignited,
			damage_pluspercent_per_10_levels, level,
			damage_pluspercent_per_10_rampage_stacks, current_rampage_stacks,
			damage_pluspercent_per_bloodline_damage_charge, current_bloodline_damage_charges,
			skill_is_fire_skill, damage_with_fire_skills_pluspercent,
			skill_is_cold_skill, damage_with_cold_skills_pluspercent,
			skill_is_lightning_skill, damage_with_lightning_skills_pluspercent,
			damage_pluspercent_while_leeching, is_leeching,
			damage_pluspercent_per_equipped_magic_item, number_of_equipped_magic_items,
			skill_is_vaal_skill, vaal_skill_damage_pluspercent,
			is_dead, damage_pluspercent_while_dead,
			number_of_active_curses_on_self, damage_pluspercent_per_active_curse_on_self,
			damage_pluspercent_with_movement_skills, skill_is_movement_skill,
			damage_pluspercent_when_currently_has_no_energy_shield, currently_has_no_energy_shield,
			damage_pluspercent_when_on_burning_ground, is_on_ground_fire_burn,
			damage_pluspercent_while_fortified, has_fortify,
			damage_pluspercent_on_consecrated_ground, on_consecrated_ground,
			damage_pluspercent_when_not_on_low_life,
			damage_pluspercent_while_totem_active, number_of_active_totems,
			damage_pluspercent_per_active_trap, number_of_active_traps,
			damage_while_no_damage_taken_pluspercent,
			virtual_minion_damage_pluspercent, modifiers_to_minion_damage_also_affect_you,
			on_full_energy_shield, damage_pluspercent_while_es_not_full,
			damage_pluspercent_for_each_trap_and_mine_active, number_of_active_mines, number_of_active_traps,
			using_flask, damage_pluspercent_during_flask_effect,
			blink_arrow_and_blink_arrow_clone_damage_pluspercent, active_skill_index,
			mirror_arrow_and_mirror_arrow_clone_damage_pluspercent,
			damage_pluspercent_if_you_have_consumed_a_corpse_recently, number_of_corpses_consumed_recently,
			minion_damage_increases_and_reductions_also_affects_you, virtual_minion_damage_pluspercent )
		{
			return stats.GetStat( damage_pluspercent ) +
				stats.GetStat( backstab_damage_pluspercent ) +
				( stats.GetStat( damage_pluspercent_per_equipped_magic_item ) * stats.GetStat( number_of_equipped_magic_items ) ) +
				( stats.GetStat( damage_pluspercent_per_active_trap ) * stats.GetStat( number_of_active_traps ) ) +
				( stats.GetStat( minion_damage_increases_and_reductions_also_affects_you ) ? stats.GetStat( virtual_minion_damage_pluspercent ) : 0 ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_damage_pluspercent ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? stats.GetStat( virtual_minion_damage_pluspercent ) : 0 ) +
				( stats.GetStat( number_of_active_totems ) ? stats.GetStat( damage_pluspercent_while_totem_active ) : 0 ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_damage_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) : 0 ) +
				( stats.GetStat( is_projectile ) && stats.GetStat( arrow_pierce_chance_applies_to_projectile_damage ) ? stats.GetStat( arrow_pierce_percent ) : 0 ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( damage_pluspercent_when_on_low_life ) : stats.GetStat( damage_pluspercent_when_not_on_low_life ) ) +
				( stats.GetStat( on_full_life ) ? stats.GetStat( damage_pluspercent_when_on_full_life ) : 0 ) +
				( stats.GetStat( is_area_damage ) ? stats.GetStat( area_damage_pluspercent ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( damage_pluspercent_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( damage_while_no_damage_taken_pluspercent ) ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( damage_pluspercent_while_fortified ) : 0 ) +
				( stats.GetStat( current_frenzy_charges ) * stats.GetStat( damage_pluspercent_per_frenzy_charge ) ) +
				( stats.GetStat( current_endurance_charges ) * stats.GetStat( damage_pluspercent_per_endurance_charge ) ) +
				( stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_damage_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_damage_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_damage_pluspercent ) : 0 ) +
				( stats.GetStat( is_shocked ) ? stats.GetStat( damage_pluspercent_per_shock ) : 0 ) +
				( stats.GetStat( number_of_corpses_consumed_recently ) ? stats.GetStat( damage_pluspercent_if_you_have_consumed_a_corpse_recently ) : 0 ) +
				( stats.GetStat( is_ignited ) ? stats.GetStat( damage_pluspercent_while_ignited ) : 0 ) +
				( stats.GetStat( is_leeching ) ? stats.GetStat( damage_pluspercent_while_leeching ) : 0 ) +
				( stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_damage_pluspercent ) : 0 ) +
				( stats.GetStat( is_dead ) ? stats.GetStat( damage_pluspercent_while_dead ) : 0 ) +
				Divide( stats.GetStat( strength ), stats.GetStat( damage_plus1percent_per_X_strength ) ) +
				( stats.GetStat( damage_pluspercent_per_10_levels ) * stats.GetStat( level ) / 10 ) +
				( stats.GetStat( damage_pluspercent_per_10_rampage_stacks ) * ( stats.GetStat( current_rampage_stacks ) / 20 ) ) + //Actually per 20
				( stats.GetStat( damage_pluspercent_per_bloodline_damage_charge ) * ( stats.GetStat( current_bloodline_damage_charges ) ) ) +
				( stats.GetStat( skill_is_fire_skill ) ? stats.GetStat( damage_with_fire_skills_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_cold_skill ) ? stats.GetStat( damage_with_cold_skills_pluspercent ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( damage_pluspercent_during_flask_effect ) : 0 ) +
				( stats.GetStat( skill_is_lightning_skill ) ? stats.GetStat( damage_with_lightning_skills_pluspercent ) : 0 ) +
				( stats.GetStat( number_of_active_curses_on_self ) * stats.GetStat( damage_pluspercent_per_active_curse_on_self ) ) +
				( stats.GetStat( skill_is_movement_skill ) ? stats.GetStat( damage_pluspercent_with_movement_skills ) : 0 ) +
				( stats.GetStat( is_on_ground_fire_burn ) ? stats.GetStat( damage_pluspercent_when_on_burning_ground ) : 0 ) +
				( stats.GetStat( currently_has_no_energy_shield ) ? stats.GetStat( damage_pluspercent_when_currently_has_no_energy_shield ) : 0 ) +
				( stats.GetStat( current_frenzy_charges ) ? 0 : stats.GetStat( damage_while_no_frenzy_charges_pluspercent ) ) +
				( stats.GetStat( on_full_energy_shield ) ? 0 : stats.GetStat( damage_pluspercent_while_es_not_full ) ) +
				( stats.GetStat( damage_pluspercent_for_each_trap_and_mine_active ) * ( stats.GetStat( number_of_active_mines ) + stats.GetStat( number_of_active_traps ) ) ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::blink_arrow ) ? stats.GetStat( blink_arrow_and_blink_arrow_clone_damage_pluspercent ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::mirror_arrow ) ? stats.GetStat( mirror_arrow_and_mirror_arrow_clone_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_all_damage_pluspercent_final,
			active_skill_damage_pluspercent_final,
			from_code_active_skill_damage_pluspercent_final,
			support_chain_damage_pluspercent_final,
			enfeeble_damage_pluspercent_final,
			damage_pluspercent_final_for_minion,
			support_totem_damage_pluspercent_final,
			support_gem_mine_damage_pluspercent_final, skill_is_mined,
			unique_mine_damage_pluspercent_final,
			support_trap_damage_pluspercent_final, skill_is_trapped,
			trap_damage_buildup_damage_pluspercent_final,
			support_multithrow_damage_pluspercent_final,
			map_hidden_monster_damage_pluspercent_final,
			support_area_concentrate_area_damage_pluspercent_final, is_area_damage,
			support_multiple_projectile_damage_pluspercent_final, is_projectile,
			support_lesser_multiple_projectile_damage_pluspercent_final,
			support_split_projectile_damage_pluspercent_final,
			support_pierce_projectile_damage_pluspercent_final,
			support_return_projectile_damage_pluspercent_final,
			support_return_projectile_damage_pluspercent_final,
			support_clustertrap_damage_pluspercent_final,
			support_fork_projectile_damage_pluspercent_final,
			active_skill_projectile_damage_pluspercent_final,
			slayer_ascendancy_melee_splash_damage_pluspercent_final,
			support_melee_splash_damage_pluspercent_final,
			projectile_damage_pluspercent_final_from_map,
			damage_pluspercent_final_from_distance,
			cleave_damage_pluspercent_final_while_dual_wielding, is_dual_wielding,
			cast_on_damage_taken_damage_pluspercent_final,
			level_33_or_lower_damage_pluspercent_final, level,
			support_echo_damage_pluspercent_final,
			cast_on_death_damage_pluspercent_final_while_dead, is_dead,
			active_skill_area_damage_pluspercent_final,
			monster_penalty_against_minions_damage_pluspercent_final,
			support_hypothermia_damage_pluspercent_final,
			damage_pluspercent_per_frenzy_charge_final, current_frenzy_charges,
			damage_pluspercent_per_izaro_charge_final, current_izaro_charges,
			support_slower_projectiles_damage_pluspercent_final,
			support_elemental_proliferation_damage_pluspercent_final,
			damage_pluspercent_final_from_support_minion_damage,
			support_trap_and_mine_damage_pluspercent_final,
			support_minefield_mine_damage_pluspercent_final,
			support_reduced_duration_damage_pluspercent_final,
			totem_damage_pluspercent_final_per_active_totem, number_of_active_totems, skill_is_totemified,
			support_minion_damage_pluspercent_final, modifiers_to_minion_damage_also_affect_you, active_skill_minion_damage_pluspercent_final,
			berserker_damage_pluspercent_final,
			from_totem_aura_damage_pluspercent_final,
			damage_pluspercent_if_enemy_killed_recently_final, have_killed_in_past_4_seconds,
			damage_pluspercent_final_from_damaging_non_taunt_target,
			covered_in_spiders_damage_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( covered_in_spiders_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( active_skill_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( damage_pluspercent_final_from_damaging_non_taunt_target ) ) *
				( stats.GetStat( have_killed_in_past_4_seconds ) ? Scale( 100 + stats.GetStat( damage_pluspercent_if_enemy_killed_recently_final ) ) : 1 ) *
				Scale( 100 + stats.GetStat( damage_pluspercent_per_frenzy_charge_final ) * stats.GetStat( current_frenzy_charges ) ) *
				Scale( 100 + stats.GetStat( damage_pluspercent_per_izaro_charge_final ) * stats.GetStat( current_izaro_charges ) ) *
				Scale( 100 + stats.GetStat( from_code_active_skill_damage_pluspercent_final ) ) *
				( stats.GetStat( skill_is_totemified ) ? Scale( 100 + ( stats.GetStat( totem_damage_pluspercent_final_per_active_totem ) * stats.GetStat( number_of_active_totems ) ) ) : 1.0f ) *
				Scale( 100 + stats.GetStat( support_chain_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( enfeeble_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( damage_pluspercent_final_for_minion ) ) *
				Scale( 100 + stats.GetStat( support_totem_damage_pluspercent_final ) ) *
				( stats.GetStat( skill_is_mined ) ? Scale( 100 + stats.GetStat( support_gem_mine_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( skill_is_mined ) ? Scale( 100 + stats.GetStat( unique_mine_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? Scale( 100 + stats.GetStat( support_minion_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( modifiers_to_minion_damage_also_affect_you ) ? Scale( 100 + stats.GetStat( active_skill_minion_damage_pluspercent_final ) ) : 1 ) *
				Scale( 100 + stats.GetStat( support_melee_splash_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( slayer_ascendancy_melee_splash_damage_pluspercent_final ) ) *
				( stats.GetStat( skill_is_trapped ) ?
					(
						Scale( 100 + stats.GetStat( support_trap_damage_pluspercent_final ) ) *	
						Scale( 100 + stats.GetStat( trap_damage_buildup_damage_pluspercent_final ) )
						) : 1 ) *
				Scale( 100 + stats.GetStat( support_clustertrap_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_multithrow_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( map_hidden_monster_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( damage_pluspercent_final_from_distance ) ) *
				Scale( 100 + stats.GetStat( cast_on_damage_taken_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_echo_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( monster_penalty_against_minions_damage_pluspercent_final ) ) *
				( stats.GetStat( is_dual_wielding ) ? Scale( 100 + stats.GetStat( cleave_damage_pluspercent_final_while_dual_wielding ) ) : 1 ) *
				( stats.GetStat( is_area_damage ) ? Scale( 100 + stats.GetStat( support_area_concentrate_area_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( is_area_damage ) ? Scale( 100 + stats.GetStat( active_skill_area_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( level ) <= 33 ? Scale( 100 + stats.GetStat( level_33_or_lower_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( is_dead ) ? Scale( 100 + stats.GetStat( cast_on_death_damage_pluspercent_final_while_dead ) ) : 1 ) *
				( stats.GetStat( is_projectile ) ?
					(
						Scale( 100 + stats.GetStat( support_multiple_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( support_pierce_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( support_lesser_multiple_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( support_split_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( support_return_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( support_fork_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( active_skill_projectile_damage_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( projectile_damage_pluspercent_final_from_map ) ) *
						Scale( 100 + stats.GetStat( support_slower_projectiles_damage_pluspercent_final ) )
						) : 1 ) *
				Scale( 100 + stats.GetStat( berserker_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_hypothermia_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_elemental_proliferation_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( damage_pluspercent_final_from_support_minion_damage ) ) *
				Scale( 100 + stats.GetStat( from_totem_aura_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_reduced_duration_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_minefield_mine_damage_pluspercent_final ) ) *
				( stats.GetStat( skill_is_mined ) || stats.GetStat( skill_is_trapped ) ? Scale( 100 + stats.GetStat( support_trap_and_mine_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_only_spell_all_damage_pluspercent,
			spell_damage_pluspercent,
			spell_damage_pluspercent_from_dexterity,
			spell_damage_pluspercent_per_power_charge, current_power_charges,
			spell_damage_pluspercent_per_5percent_block_chance, block_percent,
			spell_damage_pluspercent_per_level, level,
			spell_damage_pluspercent_while_es_full, on_full_energy_shield,
			spell_damage_pluspercent_while_no_mana_reserved, mana_reserved,
			spell_damage_pluspercent_while_not_low_mana, on_low_mana,
			spell_damage_pluspercent_per_10_int, intelligence,
			bonus_damage_pluspercent_from_strength, strong_casting,
			spell_staff_damage_pluspercent,
			spell_damage_pluspercent_while_holding_shield,
			spell_damage_pluspercent_while_dual_wielding,
			is_dual_wielding,
			main_hand_weapon_type,
			off_hand_weapon_type )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool has_shield = off_hand_weapon_index == Items::Shield;
			const bool has_staff = main_hand_weapon_index == Items::Staff;

			return stats.GetStat( spell_damage_pluspercent ) +
				stats.GetStat( spell_damage_pluspercent_from_dexterity ) +
				stats.GetStat( spell_damage_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) +
				stats.GetStat( spell_damage_pluspercent_per_5percent_block_chance ) * ( stats.GetStat( block_percent ) / 5 ) +
				stats.GetStat( spell_damage_pluspercent_per_level ) * stats.GetStat( level ) +
				( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( spell_damage_pluspercent_while_es_full ) : 0 ) +
				( stats.GetStat( mana_reserved ) == 0 ? stats.GetStat( spell_damage_pluspercent_while_no_mana_reserved ) : 0 ) +
				( !stats.GetStat( on_low_mana ) ? stats.GetStat( spell_damage_pluspercent_while_not_low_mana ) : 0 ) +
				stats.GetStat( spell_damage_pluspercent_per_10_int ) * ( stats.GetStat( intelligence ) / 10 ) +
				( stats.GetStat( strong_casting ) ? stats.GetStat( bonus_damage_pluspercent_from_strength ) : 0 ) +
				( has_staff ? stats.GetStat( spell_staff_damage_pluspercent ) : 0 ) +
				( has_shield ? stats.GetStat( spell_damage_pluspercent_while_holding_shield ) : 0 ) +
				( stats.GetStat( is_dual_wielding ) ? stats.GetStat( spell_damage_pluspercent_while_dual_wielding ) : 0 );
		}

		VIRTUAL_STAT( combined_only_spell_all_damage_pluspercent_final,
			active_skill_spell_damage_pluspercent_final,
			pain_attunement_keystone_spell_damage_pluspercent_final,
			righteous_fire_spell_damage_pluspercent_final,
			charged_blast_spell_damage_pluspercent_final,
			support_controlled_destruction_spell_damage_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( active_skill_spell_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_controlled_destruction_spell_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( pain_attunement_keystone_spell_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( righteous_fire_spell_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( charged_blast_spell_damage_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( combined_spell_all_damage_pluspercent,
			combined_all_damage_pluspercent,
			casting_spell,
			combined_only_spell_all_damage_pluspercent,
			triggered_spell_spell_damage_pluspercent, skill_is_triggered )
		{
			return stats.GetStat( combined_all_damage_pluspercent ) +
				stats.GetStat( combined_only_spell_all_damage_pluspercent ) +
				( ( stats.GetStat( casting_spell ) && stats.GetStat( skill_is_triggered ) ) ? stats.GetStat( triggered_spell_spell_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_spell_all_damage_pluspercent_final,
			combined_all_damage_pluspercent_final,
			combined_only_spell_all_damage_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( combined_only_spell_all_damage_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_all_damage_pluspercent,
			combined_all_damage_pluspercent,
			melee_damage_pluspercent, attack_is_melee,
			damage_pluspercent_while_unarmed, main_hand_weapon_type,
			melee_damage_pluspercent_when_on_full_life, on_full_life,
			combined_only_spell_all_damage_pluspercent, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage,
			attack_damage_pluspercent_per_level, level,
			attack_damage_pluspercent_per_450_evasion, evasion_rating,
			attack_damage_pluspercent_per_frenzy_charge, current_frenzy_charges,
			attack_damage_pluspercent_while_onslaught_active, virtual_has_onslaught,
			attack_damage_pluspercent,
			dominating_blow_skill_attack_damage_pluspercent, active_skill_index,
			melee_damage_pluspercent_per_endurance_charge, current_endurance_charges,
			melee_damage_pluspercent_while_fortified, has_fortify,
			projectile_attack_damage_pluspercent_per_200_accuracy, accuracy_rating, is_projectile,
			projectile_attack_damage_pluspercent )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( combined_all_damage_pluspercent ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_attack_damage_pluspercent_per_200_accuracy ) * ( stats.GetStat( accuracy_rating ) / 200 ) : 0 ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_attack_damage_pluspercent ) : 0 ) +
				stats.GetStat( attack_damage_pluspercent_per_level ) * stats.GetStat( level ) +
				stats.GetStat( attack_damage_pluspercent_per_450_evasion ) * stats.GetStat( evasion_rating ) / 450 +
				( stats.GetStat( attack_is_melee ) ?
					stats.GetStat( melee_damage_pluspercent ) + 
					( stats.GetStat( has_fortify ) ? stats.GetStat( melee_damage_pluspercent_while_fortified ) : 0 ) +
					stats.GetStat( melee_damage_pluspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) : 0 ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( damage_pluspercent_while_unarmed ) : 0 ) +	
				( stats.GetStat( virtual_has_onslaught ) ? stats.GetStat( attack_damage_pluspercent_while_onslaught_active ) : 0 ) +
				stats.GetStat( attack_damage_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				( ( stats.GetStat( attack_is_melee ) && stats.GetStat( on_full_life ) ) ? stats.GetStat( melee_damage_pluspercent_when_on_full_life ) : 0 ) +
				( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_all_damage_pluspercent ) : 0 ) +
				stats.GetStat( attack_damage_pluspercent ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::conversion_strike ) ? stats.GetStat( dominating_blow_skill_attack_damage_pluspercent ) : 0 );				
		}

		VIRTUAL_STAT( combined_attack_all_damage_pluspercent_final,
			combined_all_damage_pluspercent_final,
			active_skill_attack_damage_pluspercent_final,
			unique_quill_rain_weapon_damage_pluspercent_final, //currently don't bother checking for weapon, we know it's a bow because you have this stat, which is only present on that bow.
			support_multiple_attack_damage_pluspercent_final,
			monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final,
			combined_only_spell_all_damage_pluspercent_final, spell_damage_modifiers_apply_to_attack_damage,
			slam_ancestor_totem_granted_melee_damage_pluspercent_final, attack_is_melee,
			active_skill_attack_damage_final_permyriad )
		{
			const auto attack_damage_permyriad = stats.GetStat( active_skill_attack_damage_final_permyriad ) / 100.0f;
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( active_skill_attack_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( unique_quill_rain_weapon_damage_pluspercent_final ) ) *
				( stats.GetStat( attack_is_melee ) ? Scale( 100 + stats.GetStat( slam_ancestor_totem_granted_melee_damage_pluspercent_final ) ) : 1.0f ) *
				Scalef( 100 + attack_damage_permyriad ) *
				Scale( 100 + stats.GetStat( support_multiple_attack_damage_pluspercent_final ) ) *
				Scale( 100 - stats.GetStat( monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final ) ) * // When this stat is changed to apply to cast speed, should be moved into all damage stat
				( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) ? Scale( 100 + stats.GetStat( combined_only_spell_all_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_all_damage_pluspercent
			, combined_attack_all_damage_pluspercent
			, wand_damage_pluspercent_per_power_charge, current_power_charges, main_hand_weapon_type
			, wand_damage_pluspercent
			, bow_damage_pluspercent
			, base_main_hand_damage_pluspercent
			, damage_pluspercent_with_one_handed_weapons
			, damage_pluspercent_with_two_handed_weapons
			)
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const bool is_wand = main_hand_weapon_index == Items::Wand;
			const bool is_bow = main_hand_weapon_index == Items::Bow;
			const bool is_one_handed = Items::IsOneHanded[main_hand_weapon_index];
			const bool is_two_handed = Items::IsTwoHanded[main_hand_weapon_index];
			return stats.GetStat( combined_attack_all_damage_pluspercent ) +
				( ( is_one_handed ) ? stats.GetStat( damage_pluspercent_with_one_handed_weapons ) : 0 ) +
				( ( is_two_handed ) ? stats.GetStat( damage_pluspercent_with_two_handed_weapons ) : 0 ) +
				( is_wand ? stats.GetStat( wand_damage_pluspercent ) : 0 ) +
				( is_wand ? stats.GetStat( wand_damage_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) : 0 ) +
				( is_bow ? stats.GetStat( bow_damage_pluspercent ) : 0 ) +
				stats.GetStat( base_main_hand_damage_pluspercent );
		}

		VIRTUAL_STAT( combined_main_hand_attack_all_damage_pluspercent_final,
			combined_attack_all_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_all_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_off_hand_attack_all_damage_pluspercent
			, combined_attack_all_damage_pluspercent
			, wand_damage_pluspercent_per_power_charge, current_power_charges, off_hand_weapon_type
			, wand_damage_pluspercent
			, damage_pluspercent_with_one_handed_weapons
			, damage_pluspercent_with_two_handed_weapons
			)
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const bool is_one_handed = Items::IsOneHanded[off_hand_weapon_index];
			const bool is_two_handed = Items::IsTwoHanded[off_hand_weapon_index];
			const bool is_wand = off_hand_weapon_index == Items::Wand;
			return stats.GetStat( combined_attack_all_damage_pluspercent ) +
				( ( is_one_handed ) ? stats.GetStat( damage_pluspercent_with_one_handed_weapons ) : 0 ) +
				( ( is_two_handed ) ? stats.GetStat( damage_pluspercent_with_two_handed_weapons ) : 0 ) +
				( is_wand ? stats.GetStat( wand_damage_pluspercent ) : 0 ) +
				( is_wand ? stats.GetStat( wand_damage_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) : 0 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_all_damage_pluspercent_final,
			combined_attack_all_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_all_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_all_damage_over_time_pluspercent,
			combined_all_damage_pluspercent,
			damage_over_time_pluspercent,
			combined_only_spell_all_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return stats.GetStat( combined_all_damage_pluspercent ) + stats.GetStat( damage_over_time_pluspercent ) +
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? stats.GetStat( combined_only_spell_all_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_all_damage_over_time_pluspercent_final,
			combined_all_damage_pluspercent_final,
			support_rapid_decay_damage_over_time_pluspercent_final,
			essence_support_damage_over_time_pluspercent_final,
			combined_only_spell_all_damage_pluspercent_final, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_all_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( essence_support_damage_over_time_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_rapid_decay_damage_over_time_pluspercent_final ) ) *
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( combined_only_spell_all_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		//physical damage
		VIRTUAL_STAT( combined_physical_damage_pluspercent,
			physical_damage_pluspercent,
			physical_damage_pluspercent_per_frenzy_charge, current_frenzy_charges,
			physical_damage_per_endurance_charge_pluspercent, current_endurance_charges,
			physical_damage_pluspercent_while_life_leeching, is_life_leeching,
			physical_damage_pluspercent_while_frozen, is_frozen,
			current_number_of_stone_golems, damage_pluspercent_of_each_type_that_you_have_an_active_golem_of )
		{
			return stats.GetStat( physical_damage_pluspercent ) +
				( stats.GetStat( physical_damage_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
				( stats.GetStat( physical_damage_per_endurance_charge_pluspercent ) * stats.GetStat( current_endurance_charges ) ) +
				( stats.GetStat( is_life_leeching ) ? stats.GetStat( physical_damage_pluspercent_while_life_leeching ) : 0 ) +
				( stats.GetStat( is_frozen ) ? stats.GetStat( physical_damage_pluspercent_while_frozen ) : 0 ) + 
				( stats.GetStat( current_number_of_stone_golems ) ? stats.GetStat( damage_pluspercent_of_each_type_that_you_have_an_active_golem_of ) : 0 );
		}

		VIRTUAL_STAT( combined_physical_damage_pluspercent_final,
			active_skill_physical_damage_pluspercent_final,
			physical_damage_pluspercent_while_at_maximum_frenzy_charges_final,
			current_frenzy_charges, max_frenzy_charges )
		{
			const auto max_frenzy = stats.GetStat( current_frenzy_charges ) == stats.GetStat( max_frenzy_charges );

			return Round( 100 *
				Scale( 100 + stats.GetStat( active_skill_physical_damage_pluspercent_final ) ) * 
				( max_frenzy ? Scale( 100 + stats.GetStat( physical_damage_pluspercent_while_at_maximum_frenzy_charges_final ) ) : 1 )
				- 100 );

			//return stats.GetStat( active_skill_physical_damage_pluspercent_final );
		}

		/*
		//There are currently no stats that increase spell physical damage specifically. If/when some are added, these will be needed, and will need to be uncommented in the spell, attack & dot versions of the combined stats.

		VIRTUAL_STAT( combined_only_spell_physical_damage_pluspercent,
			other_stat )
		{
			return stats.GetStat( other_stat );
		}

		VIRTUAL_STAT( combined_only_spell_physical_damage_pluspercent_final,
			other_stats )
		{
			return stats.GetStat( other_stat );
		}
		*/

		VIRTUAL_STAT( combined_spell_physical_damage_pluspercent,
			combined_physical_damage_pluspercent,
			/*combined_only_spell_physical_damage_pluspercent*/ )
		{
			return stats.GetStat( combined_physical_damage_pluspercent ) /* +
				stats.GetStat( combined_only_spell_physical_damage_pluspercent )*/;
		}

		VIRTUAL_STAT( combined_spell_physical_damage_pluspercent_final,
			combined_physical_damage_pluspercent_final,
			/*combined_only_spell_physical_damage_pluspercent_final*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) ) /*  *
				Scale( 100 + stats.GetStat( combined_only_spell_physical_damage_pluspercent_final ) )*/
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_physical_damage_pluspercent,
			combined_physical_damage_pluspercent,
			physical_attack_damage_pluspercent,
			bonus_damage_pluspercent_from_strength, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage, strong_casting, is_projectile,
			keystone_strong_bowman,
			physical_damage_while_dual_wielding_pluspercent, is_dual_wielding,
			melee_physical_damage_pluspercent, attack_is_melee,
			melee_physical_damage_pluspercent_while_holding_shield, off_hand_weapon_type,
			unarmed_melee_physical_damage_pluspercent, main_hand_weapon_type,
			melee_physical_damage_pluspercent_while_fortify_is_active, has_fortify,
			physical_attack_damage_pluspercent_while_holding_a_shield,
			melee_physical_damage_pluspercent_per_10_dexterity, dexterity
			//current_number_of_stone_golems, damage_pluspercent_of_each_type_that_you_have_an_active_golem_of, 
			/*combined_only_spell_physical_damage_pluspercent */ )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool unarmed = main_hand_weapon_index == Items::Unarmed;
			const bool melee = !!stats.GetStat( attack_is_melee );
			const bool projectile = !!stats.GetStat( is_projectile );

			const bool strength_applies_to_all_spell_damage = stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage );
			const bool strength_applies_to_all_attack_damage = strength_applies_to_all_spell_damage && stats.GetStat( strong_casting ); //if this is set, then the bonus from strength is applied in attack_all_damage, and should not be applied again here

			return stats.GetStat( combined_physical_damage_pluspercent ) +
				stats.GetStat( physical_attack_damage_pluspercent ) +
				( ( !strength_applies_to_all_attack_damage && melee ) ? stats.GetStat( bonus_damage_pluspercent_from_strength ) : 0 ) +
				( ( !strength_applies_to_all_attack_damage && stats.GetStat( keystone_strong_bowman ) && projectile ) ? stats.GetStat( bonus_damage_pluspercent_from_strength ) : 0 ) +
				( stats.GetStat( is_dual_wielding ) ? stats.GetStat( physical_damage_while_dual_wielding_pluspercent ) : 0 ) +
				/*( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_physical_damage_pluspercent ) : 0 ) +*/
				( stats.GetStat( attack_is_melee ) ? stats.GetStat( melee_physical_damage_pluspercent ) : 0 ) +
				( stats.GetStat( attack_is_melee ) ? stats.GetStat( melee_physical_damage_pluspercent_per_10_dexterity ) * int( stats.GetStat( dexterity ) / 10 ) : 0 ) +
				( ( off_hand_weapon_index == Items::Shield && stats.GetStat( attack_is_melee ) ) ? stats.GetStat( melee_physical_damage_pluspercent_while_holding_shield ) : 0 ) +
				( ( off_hand_weapon_index == Items::Shield ? stats.GetStat( physical_attack_damage_pluspercent_while_holding_a_shield ) : 0 ) ) +
				( ( unarmed && stats.GetStat( attack_is_melee ) ) ? stats.GetStat( unarmed_melee_physical_damage_pluspercent ) : 0 ) +
				( ( melee && stats.GetStat( has_fortify ) ) ? stats.GetStat( melee_physical_damage_pluspercent_while_fortify_is_active ) : 0 ); // +
			//  ( stats.GetStat( current_number_of_stone_golems ) ? stats.GetStat( damage_pluspercent_of_each_type_that_you_have_an_active_golem_of ) : 0 );
		}

		VIRTUAL_STAT( combined_attack_physical_damage_pluspercent_final,
			combined_physical_damage_pluspercent_final,
			phase_run_melee_physical_damage_pluspercent_final, attack_is_melee,
			support_melee_physical_damage_pluspercent_final,
			support_melee_physical_damage_pluspercent_final_while_on_full_life, on_full_life,
			support_projectile_attack_physical_damage_pluspercent_final, is_projectile,
			dual_wield_inherent_physical_attack_damage_pluspercent_final, is_dual_wielding,
			unique_lions_roar_melee_physical_damage_pluspercent_final,
			support_bloodlust_melee_physical_damage_pluspercent_final,
			newpunishment_melee_physical_damage_pluspercent_final
			/*combined_only_spell_physical_damage_pluspercent_final, spell_damage_modifiers_apply_to_attack_damage*/ )
		{
			const bool full_life = !!stats.GetStat( on_full_life );
			const bool melee = !!stats.GetStat( attack_is_melee );
			const bool projectile = !!stats.GetStat( is_projectile );

			return  Round( 100 *
				Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) ) *
				( ( melee && full_life ) ? Scale( 100 + stats.GetStat( support_melee_physical_damage_pluspercent_final_while_on_full_life ) ) : 1 ) *
				( melee ? Scale( 100 + stats.GetStat( phase_run_melee_physical_damage_pluspercent_final ) ) : 1 ) *
				/*( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) ? Scale( 100 + stats.GetStat( combined_only_spell_physical_damage_pluspercent_final ) ) : 1 ) *  */
				( melee ? Scale( 100 + stats.GetStat( support_melee_physical_damage_pluspercent_final ) ) : 1 ) *
				( stats.GetStat( is_dual_wielding ) ? Scale( 100 + stats.GetStat( dual_wield_inherent_physical_attack_damage_pluspercent_final ) ) : 1 ) *
				( projectile ? Scale( 100 + stats.GetStat( support_projectile_attack_physical_damage_pluspercent_final ) ) : 1 ) *
				( melee ? Scale( 100 + stats.GetStat( unique_lions_roar_melee_physical_damage_pluspercent_final ) ) : 1 ) *
				( melee ? Scale( 100 + stats.GetStat( support_bloodlust_melee_physical_damage_pluspercent_final ) ) : 1 ) *
				( melee ? Scale( 100 + stats.GetStat( newpunishment_melee_physical_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		const Loaders::StatsValues::Stats weapon_class_damage_increase_stats[Items::NumWeaponClasses + 1] =
		{
			physical_sword_damage_pluspercent,
			physical_sword_damage_pluspercent,
			physical_sword_damage_pluspercent,
			physical_mace_damage_pluspercent,
			physical_mace_damage_pluspercent,
			physical_mace_damage_pluspercent,
			physical_wand_damage_pluspercent,
			physical_axe_damage_pluspercent,
			physical_axe_damage_pluspercent,
			physical_bow_damage_pluspercent,
			physical_dagger_damage_pluspercent,
			physical_staff_damage_pluspercent,
			physical_claw_damage_pluspercent,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};

		const Loaders::StatsValues::Stats varunastra_damage_increase_stats[Items::NumWeaponClasses + 1] =
		{
			physical_sword_damage_pluspercent,
			physical_mace_damage_pluspercent,
			physical_axe_damage_pluspercent,
			physical_dagger_damage_pluspercent,
			physical_claw_damage_pluspercent
		};

		VIRTUAL_STAT( combined_main_hand_attack_physical_damage_pluspercent,
			combined_attack_physical_damage_pluspercent,
			weapon_physical_damage_pluspercent,
			two_handed_melee_physical_damage_pluspercent, main_hand_weapon_type,
			one_handed_melee_physical_damage_pluspercent,
			ranged_weapon_physical_damage_pluspercent,
			physical_axe_damage_pluspercent,
			physical_staff_damage_pluspercent,
			physical_claw_damage_pluspercent,
			physical_dagger_damage_pluspercent,
			physical_mace_damage_pluspercent,
			physical_bow_damage_pluspercent,
			physical_sword_damage_pluspercent,
			physical_wand_damage_pluspercent,
			physical_weapon_damage_pluspercent_per_10_str, strength,
			physical_claw_damage_pluspercent_when_on_low_life, on_low_life,
			main_hand_weapon_physical_damage_pluspercent_per_250_evasion, evasion_rating,
			modifiers_to_claw_damage_also_affect_unarmed_damage,
			bow_physical_damage_pluspercent_while_holding_shield, off_hand_weapon_type,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[main_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && Items::IsMelee[main_hand_weapon_index];
			const bool is_ranged_weapon = is_weapon && !is_melee_weapon;
			const bool is_two_handed = Items::IsTwoHanded[main_hand_weapon_index];
			const bool is_one_handed = Items::IsOneHanded[main_hand_weapon_index];
			const bool low_life = !!stats.GetStat( on_low_life );

			unsigned weapon_specific_damage_increase = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_damage_increase_stats ), std::end( varunastra_damage_increase_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_damage_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto weapon_specific_physical_damage_stat = weapon_class_damage_increase_stats[main_hand_weapon_index] )
					weapon_specific_damage_increase = stats.GetStat( weapon_specific_physical_damage_stat );
			}


			if ( main_hand_weapon_index == Items::Unarmed )
			{
				if ( stats.GetStat( modifiers_to_claw_damage_also_affect_unarmed_damage ) )
				{
					weapon_specific_damage_increase += stats.GetStat( weapon_class_damage_increase_stats[Items::Claw] );
				}
			}

			return stats.GetStat( combined_attack_physical_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( main_hand_weapon_physical_damage_pluspercent_per_250_evasion ) * stats.GetStat( evasion_rating ) / 250 : 0 ) +
				( ( is_two_handed && is_melee_weapon ) ? stats.GetStat( two_handed_melee_physical_damage_pluspercent ) : 0 ) +
				( ( is_one_handed && is_melee_weapon ) ? stats.GetStat( one_handed_melee_physical_damage_pluspercent ) : 0 ) +
				( is_ranged_weapon ? stats.GetStat( ranged_weapon_physical_damage_pluspercent ) : 0 ) +
				( is_weapon ? stats.GetStat( physical_weapon_damage_pluspercent_per_10_str ) * stats.GetStat( strength ) / 10 : 0 ) +
				( is_weapon ? stats.GetStat( weapon_physical_damage_pluspercent ) : 0 ) +
				( ( low_life && ( main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Claw || ( stats.GetStat( modifiers_to_claw_damage_also_affect_unarmed_damage ) && main_hand_weapon_index == Items::Unarmed ) ) ) ? stats.GetStat( physical_claw_damage_pluspercent_when_on_low_life ) : 0 ) +
				( ( main_hand_weapon_index == Items::Bow && off_hand_weapon_index == Items::Shield ) ? stats.GetStat( bow_physical_damage_pluspercent_while_holding_shield ) : 0 ) +
				weapon_specific_damage_increase;
		}

		VIRTUAL_STAT( combined_main_hand_attack_physical_damage_pluspercent_final,
			combined_attack_physical_damage_pluspercent_final,
			unique_facebreaker_unarmed_physical_damage_pluspercent_final, main_hand_weapon_type )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const bool unarmed = main_hand_weapon_index == Items::Unarmed;
			return  Round( 100 *
				Scale( 100 + stats.GetStat( combined_attack_physical_damage_pluspercent_final ) ) *
				( unarmed ? Scale( 100 + stats.GetStat( unique_facebreaker_unarmed_physical_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_physical_damage_pluspercent,
			combined_attack_physical_damage_pluspercent,
			weapon_physical_damage_pluspercent,
			two_handed_melee_physical_damage_pluspercent, off_hand_weapon_type,
			one_handed_melee_physical_damage_pluspercent,
			ranged_weapon_physical_damage_pluspercent,
			physical_axe_damage_pluspercent,
			physical_staff_damage_pluspercent,
			physical_claw_damage_pluspercent,
			physical_dagger_damage_pluspercent,
			physical_mace_damage_pluspercent,
			physical_bow_damage_pluspercent,
			physical_sword_damage_pluspercent,
			physical_wand_damage_pluspercent,
			physical_weapon_damage_pluspercent_per_10_str, strength,
			physical_claw_damage_pluspercent_when_on_low_life, on_low_life,
			modifiers_to_claw_damage_also_affect_unarmed_damage,
			off_hand_weapon_physical_damage_pluspercent_per_250_evasion, evasion_rating,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[off_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && Items::IsMelee[off_hand_weapon_index];
			const bool is_ranged_weapon = is_weapon && !is_melee_weapon;
			const bool is_two_handed = Items::IsTwoHanded[off_hand_weapon_index];
			const bool is_one_handed = Items::IsOneHanded[off_hand_weapon_index];
			const bool low_life = !!stats.GetStat( on_low_life );

			unsigned weapon_specific_damage_increase = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_damage_increase_stats ), std::end( varunastra_damage_increase_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_damage_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto weapon_specific_physical_damage_stat = weapon_class_damage_increase_stats[off_hand_weapon_index] )
					weapon_specific_damage_increase = stats.GetStat( weapon_specific_physical_damage_stat );
			}


			if ( off_hand_weapon_index == Items::Unarmed )
			{
				if ( stats.GetStat( modifiers_to_claw_damage_also_affect_unarmed_damage ) )
				{
					weapon_specific_damage_increase += stats.GetStat( weapon_class_damage_increase_stats[ Items::Claw ] );
				}
			}

			return stats.GetStat( combined_attack_physical_damage_pluspercent ) +
				( ( is_two_handed && is_melee_weapon ) ? stats.GetStat( two_handed_melee_physical_damage_pluspercent ) : 0 ) +
				( ( is_one_handed && is_melee_weapon ) ? stats.GetStat( one_handed_melee_physical_damage_pluspercent ) : 0 ) +
				( is_ranged_weapon ? stats.GetStat( ranged_weapon_physical_damage_pluspercent ) : 0 ) +
				( is_weapon ? stats.GetStat( physical_weapon_damage_pluspercent_per_10_str ) * stats.GetStat( strength ) / 10 : 0 ) +
				( is_weapon ? stats.GetStat( off_hand_weapon_physical_damage_pluspercent_per_250_evasion ) * stats.GetStat( evasion_rating ) / 250 : 0 ) +
				( is_weapon ? stats.GetStat( weapon_physical_damage_pluspercent ) : 0 ) +
				( low_life && ( off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Claw ) ? stats.GetStat( physical_claw_damage_pluspercent_when_on_low_life ) : 0 ) +
				weapon_specific_damage_increase;
		}

		VIRTUAL_STAT( combined_off_hand_attack_physical_damage_pluspercent_final,
			combined_attack_physical_damage_pluspercent_final,
			unique_facebreaker_unarmed_physical_damage_pluspercent_final, off_hand_weapon_type )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool unarmed = off_hand_weapon_index == Items::Unarmed;

			return  Round( 100 *
				Scale( 100 + stats.GetStat( combined_attack_physical_damage_pluspercent_final ) ) *
				( unarmed ? Scale( 100 + stats.GetStat( unique_facebreaker_unarmed_physical_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_physical_damage_over_time_pluspercent,
			combined_physical_damage_pluspercent,
			physical_damage_over_time_pluspercent,
			physical_damage_over_time_per_10_dexterity_pluspercent, dexterity /*,
			combined_only_spell_physical_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time */ )
		{
			return stats.GetStat( combined_physical_damage_pluspercent ) +
				stats.GetStat( physical_damage_over_time_pluspercent ) +
				stats.GetStat( physical_damage_over_time_per_10_dexterity_pluspercent ) * ( ( int ) stats.GetStat( dexterity ) / 10 )/*+
			 (stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? : stats.GetStat( combined_only_spell_physical_damage_pluspercent ) : 0 ) */;
		}

		VIRTUAL_STAT( combined_physical_damage_over_time_pluspercent_final,
			combined_physical_damage_pluspercent_final /*,
			combined_only_spell_physical_damage_pluspercent_final, spell_damage_modifiers_apply_to_damage_over_time */ )
		{
			return  Round( 100 *
				Scale( 100 + stats.GetStat( combined_physical_damage_pluspercent_final ) ) /* *
				(stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( unique_facebreaker_unarmed_physical_damage_pluspercent_final ) ) : 1) */
				- 100 );
		}

		//elemental damage
		VIRTUAL_STAT( combined_elemental_damage_pluspercent,
			elemental_damage_pluspercent,
			elemental_damage_pluspercent_per_level, level,
			elemental_damage_pluspercent_per_frenzy_charge, current_frenzy_charges,
			elemental_damage_pluspercent_per_stackable_unique_jewel, number_of_stackable_unique_jewels,
			elemental_damage_pluspercent_during_flask_effect, using_flask )
		{
			return stats.GetStat( elemental_damage_pluspercent ) +
				stats.GetStat( elemental_damage_pluspercent_per_level ) * stats.GetStat( level ) +
				( stats.GetStat( elemental_damage_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
				( stats.GetStat( elemental_damage_pluspercent_per_stackable_unique_jewel ) * stats.GetStat( number_of_stackable_unique_jewels ) ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( elemental_damage_pluspercent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( combined_elemental_damage_pluspercent_final,
			active_skill_elemental_damage_pluspercent_final,
			inquisitor_aura_elemental_damage_pluspercent_final,
			keystone_elemental_overload_damage_pluspercent_final,
			support_gem_elemental_damage_pluspercent_final,
			essence_support_elemental_damage_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( active_skill_elemental_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( inquisitor_aura_elemental_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( keystone_elemental_overload_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( essence_support_elemental_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_gem_elemental_damage_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( combined_only_spell_elemental_damage_pluspercent,
			spell_elemental_damage_pluspercent )
		{
			return stats.GetStat( spell_elemental_damage_pluspercent );
		}

		/*
		//There are currently no multiplicative stats that increase spell elemental damage specifically. If/when some are added, this will be needed, and will need to be uncommented in the spell, dot and attack versions of the combined stats.

		VIRTUAL_STAT( combined_only_spell_elemental_damage_pluspercent_final,
			other_stats )
		{
			return stats.GetStat( other_stat );
		}
		*/

		VIRTUAL_STAT( combined_spell_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent,
			combined_only_spell_elemental_damage_pluspercent )
		{
			return stats.GetStat( combined_elemental_damage_pluspercent ) +
				stats.GetStat( combined_only_spell_elemental_damage_pluspercent );
		}

		VIRTUAL_STAT( combined_spell_elemental_damage_pluspercent_final,
			combined_elemental_damage_pluspercent_final,
			/*combined_only_spell_elemental_damage_pluspercent_final*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) ) /*  *
				Scale( 100 + stats.GetStat( combined_only_spell_elemental_damage_pluspercent_final ) )*/
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_elemental_damage_pluspercent,
			combined_elemental_damage_pluspercent,
			combined_only_spell_elemental_damage_pluspercent, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage )
		{
			return stats.GetStat( combined_elemental_damage_pluspercent ) +
				( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_elemental_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_attack_elemental_damage_pluspercent_final,
			combined_elemental_damage_pluspercent_final,
			/*combined_only_spell_elemental_damage_pluspercent_final, spell_damage_modifiers_apply_to_attack_damage*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) ) /*  *
				( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) ? Scale( 100 + stats.GetStat( combined_only_spell_elemental_damage_pluspercent_final ) ) : 1 ) */
				- 100 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_elemental_damage_pluspercent,
			combined_attack_elemental_damage_pluspercent,
			wand_elemental_damage_pluspercent, main_hand_weapon_type,
			mace_elemental_damage_pluspercent,
			bow_elemental_damage_pluspercent,
			weapon_elemental_damage_pluspercent,
			weapon_elemental_damage_pluspercent_per_power_charge, current_power_charges,
			main_hand_weapon_elemental_damage_pluspercent,
			weapon_elemental_damage_pluspercent_while_using_flask, using_flask,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );

			const bool is_wand = main_hand_weapon_index == Items::Wand;
			const bool is_bow = main_hand_weapon_index == Items::Bow;
			const bool is_mace = main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::OneHandMace || main_hand_weapon_index == Items::TwoHandMace || main_hand_weapon_index == Items::Sceptre;
			const bool is_weapon = Items::IsWeapon[main_hand_weapon_index];

			return stats.GetStat( combined_attack_elemental_damage_pluspercent ) +
				( is_wand ? stats.GetStat( wand_elemental_damage_pluspercent ) : 0 ) +
				( is_mace ? stats.GetStat( mace_elemental_damage_pluspercent ) : 0 ) +
				( is_bow ? stats.GetStat( bow_elemental_damage_pluspercent ) : 0 ) +
				( is_weapon ? stats.GetStat( weapon_elemental_damage_pluspercent ) : 0 ) +
				( is_weapon ? stats.GetStat( weapon_elemental_damage_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) : 0 ) +
				( is_weapon ? stats.GetStat( main_hand_weapon_elemental_damage_pluspercent ) : 0 ) +
				( ( is_weapon && stats.GetStat( using_flask ) ) ? stats.GetStat( weapon_elemental_damage_pluspercent_while_using_flask ) : 0 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_elemental_damage_pluspercent_final,
			combined_attack_elemental_damage_pluspercent_final,
			support_weapon_elemental_damage_pluspercent_final, main_hand_weapon_type )
		{
			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];

			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_attack_elemental_damage_pluspercent_final ) ) *
				( is_weapon ? Scale( 100 + stats.GetStat( support_weapon_elemental_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_elemental_damage_pluspercent,
			combined_attack_elemental_damage_pluspercent,
			wand_elemental_damage_pluspercent, off_hand_weapon_type,
			mace_elemental_damage_pluspercent,
			weapon_elemental_damage_pluspercent,
			weapon_elemental_damage_pluspercent_per_power_charge, current_power_charges,
			off_hand_weapon_elemental_damage_pluspercent,
			weapon_elemental_damage_pluspercent_while_using_flask, using_flask,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );

			const bool is_wand = off_hand_weapon_index == Items::Wand;
			const bool is_mace = off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::OneHandMace || off_hand_weapon_index == Items::TwoHandMace || off_hand_weapon_index == Items::Sceptre;
			const bool is_weapon = Items::IsWeapon[off_hand_weapon_index];

			return stats.GetStat( combined_attack_elemental_damage_pluspercent ) +
				( is_wand ? stats.GetStat( wand_elemental_damage_pluspercent ) : 0 ) +
				( is_mace ? stats.GetStat( mace_elemental_damage_pluspercent ) : 0 ) +
				( is_weapon ? stats.GetStat( weapon_elemental_damage_pluspercent ) : 0 ) +
				( is_weapon ? stats.GetStat( weapon_elemental_damage_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) : 0 ) +
				( is_weapon ? stats.GetStat( off_hand_weapon_elemental_damage_pluspercent ) : 0 ) +
				( ( is_weapon && stats.GetStat( using_flask ) ) ? stats.GetStat( weapon_elemental_damage_pluspercent_while_using_flask ) : 0 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_elemental_damage_pluspercent_final,
			combined_attack_elemental_damage_pluspercent_final,
			support_weapon_elemental_damage_pluspercent_final, off_hand_weapon_type )
		{
			const bool is_weapon = Items::IsWeapon[stats.GetStat( off_hand_weapon_type )];

			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_attack_elemental_damage_pluspercent_final ) ) *
				( is_weapon ? Scale( 100 + stats.GetStat( support_weapon_elemental_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_elemental_damage_over_time_pluspercent,
			combined_elemental_damage_pluspercent,
			combined_only_spell_elemental_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return stats.GetStat( combined_elemental_damage_pluspercent ) +
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? stats.GetStat( combined_only_spell_elemental_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_elemental_damage_over_time_pluspercent_final,
			combined_elemental_damage_pluspercent_final /*,
			combined_only_spell_elemental_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time */ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_elemental_damage_pluspercent_final ) ) /* *
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( combined_only_spell_elemental_damage_pluspercent ) ) : 1 ) */
				- 100 );
		}

		//fire damage
		VIRTUAL_STAT( combined_fire_damage_pluspercent,
			fire_damage_pluspercent,
			current_number_of_fire_golems, damage_pluspercent_of_each_type_that_you_have_an_active_golem_of )
		{
			return stats.GetStat( fire_damage_pluspercent ) +
				( stats.GetStat( current_number_of_fire_golems ) ? stats.GetStat( damage_pluspercent_of_each_type_that_you_have_an_active_golem_of ) : 0 );
		}

		VIRTUAL_STAT( combined_fire_damage_pluspercent_final,
			active_skill_fire_damage_pluspercent_final )
		{
			return stats.GetStat( active_skill_fire_damage_pluspercent_final );
		}


		//There are currently no stats that increase spell fire damage specifically. If/when some are added, these will be needed, and will need to be uncommented in the spell, dot and attack versions of the combined stats.

		VIRTUAL_STAT( combined_only_spell_fire_damage_pluspercent,
			spell_fire_damage_pluspercent )
		{
			return stats.GetStat( spell_fire_damage_pluspercent );
		}

		/*VIRTUAL_STAT( combined_only_spell_fire_damage_pluspercent_final,
			other_stats )
		{
			return stats.GetStat( other_stat );
		}
		*/

		VIRTUAL_STAT( combined_spell_fire_damage_pluspercent,
			combined_fire_damage_pluspercent,
			combined_only_spell_fire_damage_pluspercent )
		{
			return stats.GetStat( combined_fire_damage_pluspercent ) +
				stats.GetStat( combined_only_spell_fire_damage_pluspercent );
		}

		VIRTUAL_STAT( combined_spell_fire_damage_pluspercent_final,
			combined_fire_damage_pluspercent_final,
			/*combined_only_spell_fire_damage_pluspercent_final*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) /*  *
				Scale( 100 + stats.GetStat( combined_only_spell_fire_damage_pluspercent_final ) )*/
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_fire_damage_pluspercent,
			combined_fire_damage_pluspercent,
			combined_only_spell_fire_damage_pluspercent, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage,
			melee_fire_damage_pluspercent_while_holding_shield, fire_attack_damage_pluspercent_while_holding_a_shield, off_hand_weapon_type,
			main_hand_weapon_type,
			fire_attack_damage_pluspercent,
			fire_damage_while_dual_wielding_pluspercent, is_dual_wielding )
		{
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];
			const bool is_melee = Items::IsMelee[main_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && is_melee;
			const bool has_shield = off_hand_weapon_index == Items::Shield;

			return stats.GetStat( combined_fire_damage_pluspercent ) +
				( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_fire_damage_pluspercent ) : 0 ) +
				( ( has_shield && is_melee ) ? stats.GetStat( melee_fire_damage_pluspercent_while_holding_shield ) : 0 ) +
				( ( has_shield ) ? stats.GetStat( fire_attack_damage_pluspercent_while_holding_a_shield ) : 0 ) +
				stats.GetStat( fire_attack_damage_pluspercent ) +
				( stats.GetStat( is_dual_wielding ) ? stats.GetStat( fire_damage_while_dual_wielding_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_attack_fire_damage_pluspercent_final,
			combined_fire_damage_pluspercent_final,
			/*combined_only_spell_fire_damage_pluspercent_final, spell_damage_modifiers_apply_to_attack_damage*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) ) /*  *
				( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) ? Scale( 100 + stats.GetStat( combined_only_spell_fire_damage_pluspercent_final ) ) : 1 ) */
				- 100 );
		}

		const Loaders::StatsValues::Stats weapon_class_fire_damage_increase_stats[Items::NumWeaponClasses + 1] =
		{
			fire_sword_damage_pluspercent,
			fire_sword_damage_pluspercent,
			fire_sword_damage_pluspercent,
			fire_mace_damage_pluspercent,
			fire_mace_damage_pluspercent,
			fire_mace_damage_pluspercent,
			fire_wand_damage_pluspercent,
			fire_axe_damage_pluspercent,
			fire_axe_damage_pluspercent,
			fire_bow_damage_pluspercent,
			fire_dagger_damage_pluspercent,
			fire_staff_damage_pluspercent,
			fire_claw_damage_pluspercent,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};

		const Loaders::StatsValues::Stats varunastra_fire_damage_increase_stats[Items::NumWeaponClasses + 1] =
		{
			fire_sword_damage_pluspercent,
			fire_mace_damage_pluspercent,
			fire_axe_damage_pluspercent,
			fire_dagger_damage_pluspercent,
			fire_claw_damage_pluspercent
		};


		VIRTUAL_STAT( combined_main_hand_attack_fire_damage_pluspercent,
			combined_attack_fire_damage_pluspercent,
			weapon_fire_damage_pluspercent, main_hand_weapon_type,
			two_handed_melee_fire_damage_pluspercent,
			fire_sword_damage_pluspercent,
			fire_wand_damage_pluspercent,
			fire_staff_damage_pluspercent,
			fire_mace_damage_pluspercent,
			fire_dagger_damage_pluspercent,
			fire_claw_damage_pluspercent,
			fire_bow_damage_pluspercent,
			fire_axe_damage_pluspercent,
			melee_fire_damage_pluspercent,
			one_handed_melee_fire_damage_pluspercent,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];
			const bool is_melee = Items::IsMelee[main_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && is_melee;
			const bool is_ranged_weapon = is_weapon && !is_melee_weapon;
			const bool is_two_handed = Items::IsTwoHanded[main_hand_weapon_index];
			const bool is_one_handed = Items::IsOneHanded[main_hand_weapon_index];
			const bool is_wand = main_hand_weapon_index == Items::Wand;

			unsigned weapon_specific_damage_increase = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_fire_damage_increase_stats ), std::end( varunastra_fire_damage_increase_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_damage_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto stat = weapon_class_fire_damage_increase_stats[main_hand_weapon_index] )
					weapon_specific_damage_increase = stats.GetStat( stat );
			}

			return stats.GetStat( combined_attack_fire_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_fire_damage_pluspercent ) : 0 ) +
				weapon_specific_damage_increase +
				( ( is_melee_weapon && is_two_handed ) ? stats.GetStat( two_handed_melee_fire_damage_pluspercent ) : 0 ) +
				( is_melee ? stats.GetStat( melee_fire_damage_pluspercent ) : 0 ) +
				( ( is_one_handed && is_melee_weapon ) ? stats.GetStat( one_handed_melee_fire_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_fire_damage_pluspercent_final,
			combined_attack_fire_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_fire_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_off_hand_attack_fire_damage_pluspercent,
			combined_attack_fire_damage_pluspercent,
			weapon_fire_damage_pluspercent, off_hand_weapon_type,
			two_handed_melee_fire_damage_pluspercent,
			fire_sword_damage_pluspercent,
			fire_wand_damage_pluspercent,
			fire_staff_damage_pluspercent,
			fire_mace_damage_pluspercent,
			fire_dagger_damage_pluspercent,
			fire_claw_damage_pluspercent,
			fire_bow_damage_pluspercent,
			fire_axe_damage_pluspercent,
			melee_fire_damage_pluspercent,
			one_handed_melee_fire_damage_pluspercent,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[stats.GetStat( off_hand_weapon_type )];
			const bool is_melee = Items::IsMelee[off_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && is_melee;
			const bool is_ranged_weapon = is_weapon && !is_melee_weapon;
			const bool is_two_handed = Items::IsTwoHanded[off_hand_weapon_index];
			const bool is_one_handed = Items::IsOneHanded[off_hand_weapon_index];
			const bool is_wand = off_hand_weapon_index == Items::Wand;

			unsigned weapon_specific_damage_increase = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_fire_damage_increase_stats ), std::end( varunastra_fire_damage_increase_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_damage_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto stat = weapon_class_fire_damage_increase_stats[off_hand_weapon_index] )
					weapon_specific_damage_increase = stats.GetStat( stat );
			}

			return stats.GetStat( combined_attack_fire_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_fire_damage_pluspercent ) : 0 ) +
				weapon_specific_damage_increase +
				( ( is_melee_weapon && is_two_handed ) ? stats.GetStat( two_handed_melee_fire_damage_pluspercent ) : 0 ) +
				( is_melee ? stats.GetStat( melee_fire_damage_pluspercent ) : 0 ) +
				( ( is_one_handed && is_melee_weapon ) ? stats.GetStat( one_handed_melee_fire_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_fire_damage_pluspercent_final,
			combined_attack_fire_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_fire_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_fire_burning_damage_over_time_pluspercent,
			combined_fire_damage_pluspercent,
			burn_damage_pluspercent,
			combined_only_spell_fire_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return stats.GetStat( combined_fire_damage_pluspercent ) + stats.GetStat( burn_damage_pluspercent ) +
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? stats.GetStat( combined_only_spell_fire_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_fire_burning_damage_over_time_pluspercent_final,
			combined_fire_damage_pluspercent_final,
			unique_emberwake_burning_damage_minuspercent_final /*,
			combined_only_spell_fire_damage_pluspercent_final, spell_damage_modifiers_apply_to_damage_over_time*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_fire_damage_pluspercent_final ) )  *
				Scale( 100 - stats.GetStat( unique_emberwake_burning_damage_minuspercent_final ) ) /*  *
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( combined_only_spell_cold_damage_pluspercent_final ) ) : 1 )*/
				- 100 );
		}

		//cold damage
		VIRTUAL_STAT( combined_cold_damage_pluspercent,
			cold_damage_pluspercent,
			cold_damage_pluspercent_per_1percent_block_chance, block_percent,
			base_cold_damage_pluspercent_per_frost_nova_stack, current_frost_nova_stacks,
			current_number_of_ice_golems, damage_pluspercent_of_each_type_that_you_have_an_active_golem_of )
		{
			return stats.GetStat( cold_damage_pluspercent ) +
				stats.GetStat( cold_damage_pluspercent_per_1percent_block_chance ) * stats.GetStat( block_percent ) +
				stats.GetStat( base_cold_damage_pluspercent_per_frost_nova_stack ) * stats.GetStat( current_frost_nova_stacks ) +
				( stats.GetStat( current_number_of_ice_golems ) ? stats.GetStat( damage_pluspercent_of_each_type_that_you_have_an_active_golem_of ) : 0 );
		}

		VIRTUAL_STAT( combined_cold_damage_pluspercent_final,
			active_skill_cold_damage_pluspercent_final )
		{
			return stats.GetStat( active_skill_cold_damage_pluspercent_final );
		}

		//There are currently no stats that increase spell cold damage specifically. If/when some are added, these will be needed, and will need to be uncommented in the spell and attack versions of the combined stats.
		VIRTUAL_STAT( combined_only_spell_cold_damage_pluspercent,
			spell_cold_damage_pluspercent )
		{
			return stats.GetStat( spell_cold_damage_pluspercent );
		}

		/*VIRTUAL_STAT( combined_only_spell_cold_damage_pluspercent_final,
		other_stats )
		{
			return stats.GetStat( other_stat );
		}
		*/

		VIRTUAL_STAT( combined_spell_cold_damage_pluspercent,
			combined_cold_damage_pluspercent,
			combined_only_spell_cold_damage_pluspercent )
		{
			return stats.GetStat( combined_cold_damage_pluspercent ) +
				stats.GetStat( combined_only_spell_cold_damage_pluspercent );
		}

		VIRTUAL_STAT( combined_spell_cold_damage_pluspercent_final,
			combined_cold_damage_pluspercent_final,
			/*combined_only_spell_cold_damage_pluspercent_final*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) /*  *
				Scale( 100 + stats.GetStat( combined_only_spell_cold_damage_pluspercent_final ) )*/
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_cold_damage_pluspercent,
			combined_cold_damage_pluspercent,
			combined_only_spell_cold_damage_pluspercent, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage,
			melee_cold_damage_pluspercent_while_holding_shield, cold_attack_damage_pluspercent_while_holding_a_shield, off_hand_weapon_type,
			main_hand_weapon_type,
			cold_attack_damage_pluspercent,
			cold_damage_while_dual_wielding_pluspercent, is_dual_wielding )
		{
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];
			const bool is_melee = Items::IsMelee[main_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && is_melee;
			const bool has_shield = off_hand_weapon_index == Items::Shield;

			return stats.GetStat( combined_cold_damage_pluspercent ) +
				( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_cold_damage_pluspercent ) : 0 ) +
				( ( has_shield && is_melee ) ? stats.GetStat( melee_cold_damage_pluspercent_while_holding_shield ) : 0 ) +
				( ( has_shield ) ? stats.GetStat( cold_attack_damage_pluspercent_while_holding_a_shield ) : 0 ) +
				stats.GetStat( cold_attack_damage_pluspercent ) +
				( stats.GetStat( is_dual_wielding ) ? stats.GetStat( cold_damage_while_dual_wielding_pluspercent ) : 0 );
		}

		const Loaders::StatsValues::Stats weapon_class_cold_damage_increase_stats[Items::NumWeaponClasses + 1] =
		{
			cold_sword_damage_pluspercent,
			cold_sword_damage_pluspercent,
			cold_sword_damage_pluspercent,
			cold_mace_damage_pluspercent,
			cold_mace_damage_pluspercent,
			cold_mace_damage_pluspercent,
			cold_wand_damage_pluspercent,
			cold_axe_damage_pluspercent,
			cold_axe_damage_pluspercent,
			cold_bow_damage_pluspercent,
			cold_dagger_damage_pluspercent,
			cold_staff_damage_pluspercent,
			cold_claw_damage_pluspercent,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};

		const Loaders::StatsValues::Stats varunastra_cold_damage_increase_stats[Items::NumWeaponClasses + 1] =
		{
			cold_sword_damage_pluspercent,
			cold_mace_damage_pluspercent,
			cold_axe_damage_pluspercent,
			cold_dagger_damage_pluspercent,
			cold_claw_damage_pluspercent,
		};

		VIRTUAL_STAT( combined_main_hand_attack_cold_damage_pluspercent,
			combined_attack_cold_damage_pluspercent,
			weapon_cold_damage_pluspercent, main_hand_weapon_type,
			two_handed_melee_cold_damage_pluspercent,
			cold_sword_damage_pluspercent,
			cold_wand_damage_pluspercent,
			cold_staff_damage_pluspercent,
			cold_mace_damage_pluspercent,
			cold_dagger_damage_pluspercent,
			cold_claw_damage_pluspercent,
			cold_bow_damage_pluspercent,
			cold_axe_damage_pluspercent,
			melee_cold_damage_pluspercent,
			one_handed_melee_cold_damage_pluspercent,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];
			const bool is_melee = Items::IsMelee[main_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && is_melee;
			const bool is_ranged_weapon = is_weapon && !is_melee_weapon;
			const bool is_two_handed = Items::IsTwoHanded[main_hand_weapon_index];
			const bool is_one_handed = Items::IsOneHanded[main_hand_weapon_index];
			const bool is_wand = main_hand_weapon_index == Items::Wand;

			unsigned weapon_specific_damage_increase = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_cold_damage_increase_stats ), std::end( varunastra_cold_damage_increase_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_damage_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto stat = weapon_class_cold_damage_increase_stats[main_hand_weapon_index] )
					weapon_specific_damage_increase = stats.GetStat( stat );
			}

			return stats.GetStat( combined_attack_cold_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_cold_damage_pluspercent ) : 0 ) +
				weapon_specific_damage_increase +
				( ( is_melee_weapon && is_two_handed ) ? stats.GetStat( two_handed_melee_cold_damage_pluspercent ) : 0 ) +
				( is_melee ? stats.GetStat( melee_cold_damage_pluspercent ) : 0 ) +
				( ( is_one_handed && is_melee_weapon ) ? stats.GetStat( one_handed_melee_cold_damage_pluspercent ) : 0 );
		}


		VIRTUAL_STAT( combined_main_hand_attack_cold_damage_pluspercent_final,
			combined_attack_cold_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_cold_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_off_hand_attack_cold_damage_pluspercent,
			combined_attack_cold_damage_pluspercent,
			weapon_cold_damage_pluspercent, off_hand_weapon_type,
			two_handed_melee_cold_damage_pluspercent,
			cold_sword_damage_pluspercent,
			cold_wand_damage_pluspercent,
			cold_staff_damage_pluspercent,
			cold_mace_damage_pluspercent,
			cold_dagger_damage_pluspercent,
			cold_claw_damage_pluspercent,
			cold_bow_damage_pluspercent,
			cold_axe_damage_pluspercent,
			melee_cold_damage_pluspercent,
			one_handed_melee_cold_damage_pluspercent,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_weapon = Items::IsWeapon[stats.GetStat( off_hand_weapon_type )];
			const bool is_melee = Items::IsMelee[off_hand_weapon_index];
			const bool is_melee_weapon = is_weapon && is_melee;
			const bool is_ranged_weapon = is_weapon && !is_melee_weapon;
			const bool is_two_handed = Items::IsTwoHanded[off_hand_weapon_index];
			const bool is_one_handed = Items::IsOneHanded[off_hand_weapon_index];
			const bool is_wand = off_hand_weapon_index == Items::Wand;

			unsigned weapon_specific_damage_increase = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_cold_damage_increase_stats ), std::end( varunastra_cold_damage_increase_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_damage_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto stat = weapon_class_cold_damage_increase_stats[off_hand_weapon_index] )
					weapon_specific_damage_increase = stats.GetStat( stat );
			}

			return stats.GetStat( combined_attack_cold_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_cold_damage_pluspercent ) : 0 ) +
				weapon_specific_damage_increase +
				( ( is_melee_weapon && is_two_handed ) ? stats.GetStat( two_handed_melee_cold_damage_pluspercent ) : 0 ) +
				( is_melee ? stats.GetStat( melee_cold_damage_pluspercent ) : 0 ) +
				( ( is_one_handed && is_melee_weapon ) ? stats.GetStat( one_handed_melee_cold_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_cold_damage_pluspercent_final,
			combined_attack_cold_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_cold_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_cold_damage_over_time_pluspercent,
			combined_cold_damage_pluspercent,
			cold_damage_over_time_pluspercent,
			combined_only_spell_cold_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return stats.GetStat( combined_cold_damage_pluspercent ) +
				stats.GetStat( cold_damage_over_time_pluspercent ) +
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? stats.GetStat( combined_only_spell_cold_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_cold_damage_over_time_pluspercent_final,
			combined_cold_damage_pluspercent_final /*,
			combined_only_spell_cold_damage_pluspercent_final, spell_damage_modifiers_apply_to_damage_over_time*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_cold_damage_pluspercent_final ) ) /* *
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( combined_only_spell_cold_damage_pluspercent_final ) ) : 1 ) */
				- 100 );
		}

		//lightning damage
		VIRTUAL_STAT( combined_lightning_damage_pluspercent,
			lightning_damage_pluspercent,
			lightning_damage_pluspercent_per_frenzy_charge, current_frenzy_charges,
			lightning_damage_pluspercent_per_10_intelligence, intelligence,
			current_number_of_lightning_golems, damage_pluspercent_of_each_type_that_you_have_an_active_golem_of )
		{
			return stats.GetStat( lightning_damage_pluspercent ) +
				( stats.GetStat( lightning_damage_pluspercent_per_10_intelligence ) * ( stats.GetStat( intelligence ) / 10 ) ) +
				( stats.GetStat( current_number_of_lightning_golems ) ? stats.GetStat( damage_pluspercent_of_each_type_that_you_have_an_active_golem_of ) : 0 ) +
				stats.GetStat( lightning_damage_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges );
		}

		VIRTUAL_STAT( combined_lightning_damage_pluspercent_final,
			active_skill_lightning_damage_pluspercent_final )
		{
			return stats.GetStat( active_skill_lightning_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_only_spell_lightning_damage_pluspercent,
			base_spell_lightning_damage_pluspercent )
		{
			return stats.GetStat( base_spell_lightning_damage_pluspercent );
		}

		VIRTUAL_STAT( combined_only_spell_lightning_damage_pluspercent_final,
			wrath_aura_spell_lightning_damage_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( wrath_aura_spell_lightning_damage_pluspercent_final ) )
				- 100 );
		}


		VIRTUAL_STAT( combined_spell_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent,
			combined_only_spell_lightning_damage_pluspercent )
		{
			return stats.GetStat( combined_lightning_damage_pluspercent ) +
				stats.GetStat( combined_only_spell_lightning_damage_pluspercent );
		}

		VIRTUAL_STAT( combined_spell_lightning_damage_pluspercent_final,
			combined_lightning_damage_pluspercent_final,
			combined_only_spell_lightning_damage_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) )   *
				Scale( 100 + stats.GetStat( combined_only_spell_lightning_damage_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_lightning_damage_pluspercent,
			combined_lightning_damage_pluspercent,
			combined_only_spell_lightning_damage_pluspercent, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage )
		{
			return stats.GetStat( combined_lightning_damage_pluspercent ) +
				( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_lightning_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_attack_lightning_damage_pluspercent_final,
			combined_lightning_damage_pluspercent_final,
			combined_only_spell_lightning_damage_pluspercent_final, spell_damage_modifiers_apply_to_attack_damage )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) *
				( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) ? Scale( 100 + stats.GetStat( combined_only_spell_lightning_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_lightning_damage_pluspercent,
			combined_attack_lightning_damage_pluspercent,
			weapon_lightning_damage_pluspercent, main_hand_weapon_type )
		{
			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];

			return stats.GetStat( combined_attack_lightning_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_lightning_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_lightning_damage_pluspercent_final,
			combined_attack_lightning_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_lightning_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_off_hand_attack_lightning_damage_pluspercent,
			combined_attack_lightning_damage_pluspercent,
			weapon_lightning_damage_pluspercent, off_hand_weapon_type )
		{
			const bool is_weapon = Items::IsWeapon[stats.GetStat( off_hand_weapon_type )];

			return stats.GetStat( combined_attack_lightning_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_lightning_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_lightning_damage_pluspercent_final,
			combined_attack_lightning_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_lightning_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_lightning_damage_over_time_pluspercent,
			combined_lightning_damage_pluspercent,
			combined_only_spell_lightning_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return stats.GetStat( combined_lightning_damage_pluspercent ) +
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? stats.GetStat( combined_only_spell_lightning_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_lightning_damage_over_time_pluspercent_final,
			combined_lightning_damage_pluspercent_final,
			combined_only_spell_lightning_damage_pluspercent_final, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_lightning_damage_pluspercent_final ) ) *
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( combined_only_spell_lightning_damage_pluspercent_final ) ) : 1 )
				- 100 );
		}

		//chaos damage
		VIRTUAL_STAT( combined_chaos_damage_pluspercent,
			chaos_damage_pluspercent,
			chaos_damage_pluspercent_per_level, level,
			chaos_damage_pluspercent_per_equipped_corrupted_item, number_of_equipped_corrupted_items,
			current_number_of_chaos_golems, damage_pluspercent_of_each_type_that_you_have_an_active_golem_of )
		{
			return stats.GetStat( chaos_damage_pluspercent ) +
				( stats.GetStat( chaos_damage_pluspercent_per_level ) * stats.GetStat( level ) ) +
				( stats.GetStat( current_number_of_chaos_golems ) ? stats.GetStat( damage_pluspercent_of_each_type_that_you_have_an_active_golem_of ) : 0 ) +
				( stats.GetStat( chaos_damage_pluspercent_per_equipped_corrupted_item ) * stats.GetStat( number_of_equipped_corrupted_items ) );
		}

		VIRTUAL_STAT( combined_chaos_damage_pluspercent_final,
			active_skill_chaos_damage_pluspercent_final,
			support_void_manipulation_chaos_damage_pluspercent_final )
		{
			return stats.GetStat( active_skill_chaos_damage_pluspercent_final ) +
				stats.GetStat( support_void_manipulation_chaos_damage_pluspercent_final );
		}

		/*
		//There are currently no stats that increase spell chaos damage specifically. If/when some are added, these will be needed, and will need to be uncommented in the spell and attack versions of the combined stats.

		VIRTUAL_STAT( combined_only_spell_chaos_damage_pluspercent,
			other_stat )
		{
			return stats.GetStat( other_stat );
		}

		VIRTUAL_STAT( combined_only_spell_chaos_damage_pluspercent_final,
			other_stats )
		{
			return stats.GetStat( other_stat );
		}
		*/

		VIRTUAL_STAT( combined_spell_chaos_damage_pluspercent,
			combined_chaos_damage_pluspercent,
			/*combined_only_spell_chaos_damage_pluspercent*/ )
		{
			return stats.GetStat( combined_chaos_damage_pluspercent ) /* +
				stats.GetStat( combined_only_spell_chaos_damage_pluspercent )*/;
		}

		VIRTUAL_STAT( combined_spell_chaos_damage_pluspercent_final,
			combined_chaos_damage_pluspercent_final,
			/*combined_only_spell_chaos_damage_pluspercent_final*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_chaos_damage_pluspercent_final ) ) /*  *
				Scale( 100 + stats.GetStat( combined_only_spell_chaos_damage_pluspercent_final ) )*/
				- 100 );
		}

		VIRTUAL_STAT( combined_attack_chaos_damage_pluspercent,
			combined_chaos_damage_pluspercent,
			/*combined_only_spell_chaos_damage_pluspercent, spell_damage_modifiers_apply_to_attack_damage, additive_spell_damage_modifiers_apply_to_attack_damage*/ )
		{
			return stats.GetStat( combined_chaos_damage_pluspercent ) /* +
			 ( ( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) || stats.GetStat( additive_spell_damage_modifiers_apply_to_attack_damage ) ) ? stats.GetStat( combined_only_spell_chaos_damage_pluspercent ) : 0 )*/;
		}

		VIRTUAL_STAT( combined_attack_chaos_damage_pluspercent_final,
			combined_chaos_damage_pluspercent_final,
			/*combined_only_spell_chaos_damage_pluspercent_final, spell_damage_modifiers_apply_to_attack_damage*/ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_chaos_damage_pluspercent_final ) ) /*  *
				( stats.GetStat( spell_damage_modifiers_apply_to_attack_damage ) ? Scale( 100 + stats.GetStat( combined_only_spell_chaos_damage_pluspercent_final ) ) : 1 ) */
				- 100 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_chaos_damage_pluspercent,
			combined_attack_chaos_damage_pluspercent,
			weapon_chaos_damage_pluspercent, main_hand_weapon_type )
		{
			const bool is_weapon = Items::IsWeapon[stats.GetStat( main_hand_weapon_type )];

			return stats.GetStat( combined_attack_chaos_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_chaos_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_main_hand_attack_chaos_damage_pluspercent_final,
			combined_attack_chaos_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_chaos_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_off_hand_attack_chaos_damage_pluspercent,
			combined_attack_chaos_damage_pluspercent,
			weapon_chaos_damage_pluspercent, off_hand_weapon_type )
		{
			const bool is_weapon = Items::IsWeapon[stats.GetStat( off_hand_weapon_type )];

			return stats.GetStat( combined_attack_chaos_damage_pluspercent ) +
				( is_weapon ? stats.GetStat( weapon_chaos_damage_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( combined_off_hand_attack_chaos_damage_pluspercent_final,
			combined_attack_chaos_damage_pluspercent_final )
		{
			return stats.GetStat( combined_attack_chaos_damage_pluspercent_final );
		}

		VIRTUAL_STAT( combined_chaos_damage_over_time_pluspercent,
			combined_chaos_damage_pluspercent /*,
			combined_only_spell_chaos_damage_pluspercent, spell_damage_modifiers_apply_to_damage_over_time */ )
		{
			return stats.GetStat( combined_chaos_damage_pluspercent ) /*+
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? stats.GetStat( combined_only_spell_chaos_damage_pluspercent ) : 0 ) */;
		}

		VIRTUAL_STAT( combined_chaos_damage_over_time_pluspercent_final,
			combined_chaos_damage_pluspercent_final /*,
			combined_only_spell_chaos_damage_pluspercent_final, spell_damage_modifiers_apply_to_damage_over_time */ )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( combined_chaos_damage_pluspercent_final ) ) /*  *
				( stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) ? Scale( 100 + stats.GetStat( combined_only_spell_chaos_damage_pluspercent_final ) ) : 1 ) */
				- 100 );
		}

		/*
		PVP damage not included in the above stats, applied directly in ApplydamageServer
		*/

		VIRTUAL_STAT( combined_pvp_damage_pluspercent_final,
			pvp_damage_pluspercent_final_scale,
			support_cast_on_death_pvp_damage_pluspercent_final,
			support_cast_on_damage_taken_pvp_damage_pluspercent_final,
			support_cast_when_stunned_pvp_damage_pluspercent_final,
			support_cast_on_crit_pvp_damage_pluspercent_final,
			skill_total_pvp_damage_pluspercent_final,
			pvp_shield_damage_pluspercent_final, off_hand_weapon_type  )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const bool shield = off_hand_weapon_index == Items::Shield;

			return  Round( 100 *
				Scale( 100 + ( shield ? stats.GetStat( pvp_shield_damage_pluspercent_final ) : 0 ) ) *
				Scale( 100 + stats.GetStat( skill_total_pvp_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( pvp_damage_pluspercent_final_scale ) ) *
				Scale( 100 + stats.GetStat( support_cast_on_death_pvp_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_cast_on_damage_taken_pvp_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_cast_when_stunned_pvp_damage_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_cast_on_crit_pvp_damage_pluspercent_final ) )
				- 100 );
		}

		/*
		global added damage
		*/
		VIRTUAL_STAT( global_total_minimum_added_physical_damage,
			global_minimum_added_physical_damage,
			skill_is_trapped, skill_is_mined,
			trap_and_mine_minimum_added_physical_damage,
			counter_attacks_minimum_added_physical_damage, is_counterattack )
		{
			bool is_trap_or_mine = stats.GetStat( skill_is_trapped ) || stats.GetStat( skill_is_mined );

			return stats.GetStat( global_minimum_added_physical_damage ) +
				( is_trap_or_mine ? stats.GetStat( trap_and_mine_minimum_added_physical_damage ) : 0 ) +
				( stats.GetStat( is_counterattack ) ? stats.GetStat( counter_attacks_minimum_added_physical_damage ) : 0 );
		}

		VIRTUAL_STAT( global_total_maximum_added_physical_damage,
			global_maximum_added_physical_damage,
			skill_is_trapped, skill_is_mined,
			trap_and_mine_maximum_added_physical_damage,
			counter_attacks_maximum_added_physical_damage, is_counterattack )
		{
			bool is_trap_or_mine = stats.GetStat( skill_is_trapped ) || stats.GetStat( skill_is_mined );

			return stats.GetStat( global_maximum_added_physical_damage ) +
				( is_trap_or_mine ? stats.GetStat( trap_and_mine_maximum_added_physical_damage ) : 0 ) +
				( stats.GetStat( is_counterattack ) ? stats.GetStat( counter_attacks_maximum_added_physical_damage ) : 0 );
		}

		VIRTUAL_STAT( global_total_minimum_added_cold_damage,
			global_minimum_added_cold_damage, 
			counter_attacks_minimum_added_cold_damage, is_counterattack,
			minimum_added_cold_damage_per_frenzy_charge, current_frenzy_charges )
		{
			return stats.GetStat( global_minimum_added_cold_damage ) +
				( stats.GetStat( minimum_added_cold_damage_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
				( stats.GetStat( is_counterattack ) ? stats.GetStat( counter_attacks_minimum_added_cold_damage ) : 0 );
		}

		VIRTUAL_STAT( global_total_maximum_added_cold_damage,
			global_maximum_added_cold_damage, 
			counter_attacks_maximum_added_cold_damage, is_counterattack,
			maximum_added_cold_damage_per_frenzy_charge, current_frenzy_charges )
		{
			return stats.GetStat( global_maximum_added_cold_damage ) + 
				( stats.GetStat( maximum_added_cold_damage_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
				( stats.GetStat( is_counterattack ) ? stats.GetStat( counter_attacks_maximum_added_cold_damage ) : 0 );
		}

		 //These ones not added yet as we only have one source of global added damage for each. When implemented, will need to replace instances of the non-total global stat in other stats with these ones.
		VIRTUAL_STAT( global_total_minimum_added_fire_damage,
			global_minimum_added_fire_damage,
			minimum_added_fire_damage_if_blocked_recently, have_blocked_recently )
		{
			return stats.GetStat( global_minimum_added_fire_damage ) +
				( stats.GetStat( have_blocked_recently ) ? stats.GetStat( minimum_added_fire_damage_if_blocked_recently ) : 0 );
		}

		VIRTUAL_STAT( global_total_maximum_added_fire_damage,
			global_maximum_added_fire_damage,
			maximum_added_fire_damage_if_blocked_recently, have_blocked_recently )
		{
			return stats.GetStat( global_maximum_added_fire_damage ) +
				( stats.GetStat( have_blocked_recently ) ? stats.GetStat( maximum_added_fire_damage_if_blocked_recently ) : 0 );
		}

		VIRTUAL_STAT( global_total_minimum_added_lightning_damage,
			global_minimum_added_lightning_damage,
			enchantment_boots_minimum_added_lightning_damage_when_you_havent_killed_for_4_seconds, have_killed_in_past_4_seconds )
		{
			return stats.GetStat( global_minimum_added_lightning_damage ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_minimum_added_lightning_damage_when_you_havent_killed_for_4_seconds ) );
		}

		VIRTUAL_STAT( global_total_maximum_added_lightning_damage,
			global_maximum_added_lightning_damage,
			enchantment_boots_maximum_added_lightning_damage_when_you_havent_killed_for_4_seconds, have_killed_in_past_4_seconds )
		{
			return stats.GetStat( global_maximum_added_lightning_damage ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_maximum_added_lightning_damage_when_you_havent_killed_for_4_seconds ) );
		}
		/*
		VIRTUAL_STAT( global_total_minimum_added_chaos_damage,
			global_minimum_added_chaos_damage )
		{
			return stats.GetStat( global_minimum_added_chaos_damage );
		}

		VIRTUAL_STAT( global_total_maximum_added_chaos_damage,
			global_maximum_added_chaos_damage )
		{
			return stats.GetStat( global_maximum_added_chaos_damage );
		}
		*/

		/*
		Damage over time stats: Loaders::StatsValues::Stats for damage dealer
		*/

		VIRTUAL_STAT( physical_damage_to_deal_per_minute,
			base_physical_damage_to_deal_per_minute,
			combined_all_damage_over_time_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_physical_damage_over_time_pluspercent,
			combined_physical_damage_over_time_pluspercent_final,
			deal_no_physical_damage_over_time )
		{
			if ( stats.GetStat( deal_no_physical_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_physical_damage_to_deal_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_physical_damage_over_time_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_physical_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		VIRTUAL_STAT( fire_damage_to_deal_per_minute,
			base_fire_damage_to_deal_per_minute,
			combined_all_damage_over_time_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_elemental_damage_over_time_pluspercent,
			combined_elemental_damage_over_time_pluspercent_final,
			combined_fire_burning_damage_over_time_pluspercent,
			combined_fire_burning_damage_over_time_pluspercent_final,
			deal_no_fire_damage_over_time,
			fire_damage_over_time_pluspercent )
		{
			if ( stats.GetStat( deal_no_fire_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_fire_damage_to_deal_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_elemental_damage_over_time_pluspercent ) + stats.GetStat( combined_fire_burning_damage_over_time_pluspercent ) + stats.GetStat( fire_damage_over_time_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_fire_burning_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		VIRTUAL_STAT( projectile_ground_fire_damage_per_minute,
			base_projectile_ground_fire_damage_per_minute,
			combined_all_damage_over_time_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_elemental_damage_over_time_pluspercent,
			combined_elemental_damage_over_time_pluspercent_final,
			combined_fire_burning_damage_over_time_pluspercent,
			combined_fire_burning_damage_over_time_pluspercent_final,
			deal_no_fire_damage_over_time,
			fire_damage_over_time_pluspercent )
		{
			if ( stats.GetStat( deal_no_fire_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_projectile_ground_fire_damage_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_elemental_damage_over_time_pluspercent ) + stats.GetStat( combined_fire_burning_damage_over_time_pluspercent ) + stats.GetStat( fire_damage_over_time_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_fire_burning_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		VIRTUAL_STAT( cold_damage_to_deal_per_minute,
			base_cold_damage_to_deal_per_minute,
			combined_all_damage_over_time_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_elemental_damage_over_time_pluspercent,
			combined_elemental_damage_over_time_pluspercent_final,
			combined_cold_damage_over_time_pluspercent,
			combined_cold_damage_over_time_pluspercent_final,
			deal_no_cold_damage_over_time )
		{
			if ( stats.GetStat( deal_no_cold_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_cold_damage_to_deal_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_elemental_damage_over_time_pluspercent ) + stats.GetStat( combined_cold_damage_over_time_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_cold_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		VIRTUAL_STAT( lightning_damage_to_deal_per_minute,
			base_lightning_damage_to_deal_per_minute,
			combined_all_damage_over_time_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_elemental_damage_over_time_pluspercent,
			combined_elemental_damage_over_time_pluspercent_final,
			combined_lightning_damage_over_time_pluspercent,
			combined_lightning_damage_over_time_pluspercent_final,
			deal_no_lightning_damage_over_time )
		{
			if ( stats.GetStat( deal_no_lightning_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_lightning_damage_to_deal_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_elemental_damage_over_time_pluspercent ) + stats.GetStat( combined_lightning_damage_over_time_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_elemental_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_lightning_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		VIRTUAL_STAT( chaos_damage_to_deal_per_minute,
			base_chaos_damage_to_deal_per_minute,
			combined_all_damage_over_time_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_chaos_damage_over_time_pluspercent,
			combined_chaos_damage_over_time_pluspercent_final,
			deal_no_chaos_damage_over_time )
		{
			if ( stats.GetStat( deal_no_chaos_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_chaos_damage_to_deal_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_chaos_damage_over_time_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_chaos_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		VIRTUAL_STAT( poison_damage_to_deal_per_minute,
			base_chaos_damage_to_deal_per_minute,
			combined_all_damage_over_time_pluspercent,
			base_poison_damage_pluspercent,
			combined_all_damage_over_time_pluspercent_final,
			combined_chaos_damage_over_time_pluspercent,
			combined_chaos_damage_over_time_pluspercent_final,
			deal_no_chaos_damage_over_time )
		{
			if ( stats.GetStat( deal_no_chaos_damage_over_time ) )
				return 0;

			const auto base_damage = stats.GetStat( base_chaos_damage_to_deal_per_minute );
			const auto damage_increase = stats.GetStat( combined_all_damage_over_time_pluspercent ) + stats.GetStat( combined_chaos_damage_over_time_pluspercent ) + stats.GetStat( base_poison_damage_pluspercent );
			const auto damage_scale = Scale( 100 + stats.GetStat( combined_all_damage_over_time_pluspercent_final ) ) * Scale( 100 + stats.GetStat( combined_chaos_damage_over_time_pluspercent_final ) );
			//total
			return std::max( 0, Round(
				base_damage * Scale( 100 + damage_increase ) * damage_scale
				) );
		}

		/*
		Damage over time stats: Loaders::StatsValues::Stats for damage taker
		*/

		VIRTUAL_STAT( physical_damage_taken_per_minute,
			base_physical_damage_taken_per_minute,
			base_physical_damage_percent_of_maximum_life_taken_per_minute, maximum_life,
			base_physical_damage_taken_per_minute_per_corrupted_blood_stack, current_corrupted_blood_stacks,
			base_physical_damage_taken_per_minute_per_talisman_degen_stack, current_talisman_degen_stacks,
			base_physical_damage_taken_per_minute_per_corrupted_blood_rain_stack, current_corrupted_blood_rain_stacks,
			base_physical_damage_percent_of_maximum_energy_shield_taken_per_minute, maximum_energy_shield )

		{
			return Round( stats.GetStat( base_physical_damage_taken_per_minute ) +
				Scale( stats.GetStat( base_physical_damage_percent_of_maximum_life_taken_per_minute ) ) * stats.GetStat( maximum_life ) +
				stats.GetStat( base_physical_damage_taken_per_minute_per_corrupted_blood_stack ) * stats.GetStat( current_corrupted_blood_stacks ) +
				stats.GetStat( base_physical_damage_taken_per_minute_per_corrupted_blood_rain_stack ) * stats.GetStat( current_corrupted_blood_rain_stacks ) +
				stats.GetStat( base_physical_damage_taken_per_minute_per_talisman_degen_stack ) * stats.GetStat( current_talisman_degen_stacks ) +
				Scale( stats.GetStat( base_physical_damage_percent_of_maximum_energy_shield_taken_per_minute ) ) * stats.GetStat( maximum_energy_shield ) );
		}

		VIRTUAL_STAT( fire_damage_taken_per_minute
			, base_fire_damage_taken_per_minute
			, maximum_life, maximum_energy_shield, current_difficulty
			, base_fire_damage_percent_of_maximum_life_taken_per_minute_in_merciless
			, base_fire_damage_percent_of_maximum_es_taken_per_minute_in_merciless
			, base_fire_damage_percent_of_maximum_life_taken_per_minute_in_cruel
			, base_fire_damage_percent_of_maximum_es_taken_per_minute_in_cruel
			, base_fire_damage_percent_of_maximum_life_taken_per_minute_in_normal
			, base_fire_damage_percent_of_maximum_es_taken_per_minute_in_normal
			, base_fire_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute
			)
		{
			const long long max_life = stats.GetStat( maximum_life );
			const long long max_es = stats.GetStat( maximum_energy_shield );
			const auto difficulty = stats.GetStat( current_difficulty );

			long long out = stats.GetStat( base_fire_damage_taken_per_minute ) + ( max_life + max_es ) * ( stats.GetStat( base_fire_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute ) / 100 );

			switch ( difficulty )
			{
			case 3:
				out += max_life * ( long long ) stats.GetStat( base_fire_damage_percent_of_maximum_life_taken_per_minute_in_merciless ) / 100;
				out += max_es   * ( long long ) stats.GetStat( base_fire_damage_percent_of_maximum_es_taken_per_minute_in_merciless ) / 100;
				break;
			case 2:
				out += max_life * ( long long ) stats.GetStat( base_fire_damage_percent_of_maximum_life_taken_per_minute_in_cruel ) / 100;
				out += max_es   * ( long long ) stats.GetStat( base_fire_damage_percent_of_maximum_es_taken_per_minute_in_cruel ) / 100;
				break;
			default: case 1:
				out += max_life * ( long long ) stats.GetStat( base_fire_damage_percent_of_maximum_life_taken_per_minute_in_normal ) / 100;
				out += max_es   * ( long long ) stats.GetStat( base_fire_damage_percent_of_maximum_es_taken_per_minute_in_normal ) / 100;
				break;
			}

			return static_cast< int >( out );
		}

		VIRTUAL_STAT( nonlethal_fire_damage_taken_per_minute,
			base_nonlethal_fire_damage_percent_of_maximum_life_taken_per_minute, maximum_life,
			base_nonlethal_fire_damage_percent_of_maximum_energy_shield_taken_per_minute, maximum_energy_shield )
		{
			return static_cast< int >( stats.GetStat( maximum_life ) * ( stats.GetStat( base_nonlethal_fire_damage_percent_of_maximum_life_taken_per_minute ) / 100.0f )  +
				stats.GetStat( maximum_energy_shield ) * ( stats.GetStat( base_nonlethal_fire_damage_percent_of_maximum_energy_shield_taken_per_minute ) / 100.0f ) );
		}

		VIRTUAL_STAT( cold_damage_taken_per_minute,
			base_cold_damage_taken_per_minute,
			base_cold_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute, maximum_life, maximum_energy_shield )
		{
			return static_cast< int >( stats.GetStat( base_cold_damage_taken_per_minute ) +
				( ( stats.GetStat( maximum_life ) + stats.GetStat( maximum_energy_shield ) ) * ( stats.GetStat( base_cold_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute ) / 100.0f ) ) );
		}

		VIRTUAL_STAT( lightning_damage_taken_per_minute,
			base_lightning_damage_taken_per_minute,
			base_lightning_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute, maximum_life, maximum_energy_shield,
			base_lightning_damage_taken_per_second )
		{
			return	static_cast< int >( stats.GetStat( base_lightning_damage_taken_per_minute ) +
				( stats.GetStat( base_lightning_damage_taken_per_second ) * 60 ) +
				( ( stats.GetStat( maximum_life ) + stats.GetStat( maximum_energy_shield ) ) * ( stats.GetStat( base_lightning_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute ) / 100.0f ) ) );
		}

		VIRTUAL_STAT( chaos_damage_taken_per_minute,
			base_chaos_damage_taken_per_minute,
			base_chaos_damage_percent_of_maximum_life_taken_per_minute, maximum_life,
			base_chaos_damage_taken_per_minute_per_viper_strike_orb, current_viper_strike_orbs,
			base_chaos_damage_percent_of_maximum_life_taken_per_minute_per_frenzy_charge, current_frenzy_charges )
		{
			return Round( stats.GetStat( base_chaos_damage_taken_per_minute ) +
				Scale( stats.GetStat( base_chaos_damage_percent_of_maximum_life_taken_per_minute ) ) * stats.GetStat( maximum_life ) +
				stats.GetStat( base_chaos_damage_taken_per_minute_per_viper_strike_orb ) * stats.GetStat( current_viper_strike_orbs ) +
				Scale( stats.GetStat( base_chaos_damage_percent_of_maximum_life_taken_per_minute_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) * stats.GetStat( maximum_life ) );
		}

		/*
		Total Damage Taken stats.
		 */

		VIRTUAL_STAT( total_physical_damage_taken_per_minute,
			physical_damage_taken_per_minute,
			additional_physical_damage_reduction_percent,
			degen_effect_pluspercent,
			damage_taken_pluspercent,
			virtual_physical_damage_taken_pluspercent,
			cannot_be_damaged,
			physical_immunity )
		{
			if ( stats.GetStat( cannot_be_damaged ) || stats.GetStat( physical_immunity ) )
				return 0;

			const int damage_reduction_percent = stats.GetStat( additional_physical_damage_reduction_percent );
			const int base_amount = static_cast< int >( stats.GetStat( physical_damage_taken_per_minute ) * ( 1.0f - damage_reduction_percent / 100.0f ) ); //integer math here causes early wraparound due to multiplying value by 100
			return static_cast< int >( base_amount * ( 1.0f + (
				stats.GetStat( degen_effect_pluspercent ) +
				stats.GetStat( damage_taken_pluspercent ) +
				stats.GetStat( virtual_physical_damage_taken_pluspercent )
				) / 100.0f ) );
		}

		VIRTUAL_STAT( total_fire_damage_taken_per_minute,
			fire_damage_taken_per_minute,
			fire_damage_resistance_percent,
			degen_effect_pluspercent,
			damage_taken_pluspercent,
			virtual_elemental_damage_taken_pluspercent,
			fire_damage_taken_pluspercent,
			burning_damage_taken_pluspercent,
			cannot_be_damaged,
			fire_damage_immunity )
		{
			if ( stats.GetStat( cannot_be_damaged ) || stats.GetStat( fire_damage_immunity ) )
				return 0;

			const int damage_reduction_percent = stats.GetStat( fire_damage_resistance_percent );
			const int base_amount = static_cast< int >( stats.GetStat( fire_damage_taken_per_minute ) * ( 1.0f - damage_reduction_percent / 100.0f ) ); //integer math here causes early wraparound due to multiplying value by 100
			return static_cast< int >( base_amount * ( 1.0f + ( 
					stats.GetStat( degen_effect_pluspercent ) +
					stats.GetStat( damage_taken_pluspercent ) +
					stats.GetStat( virtual_elemental_damage_taken_pluspercent ) +
					stats.GetStat( fire_damage_taken_pluspercent ) +
					stats.GetStat( burning_damage_taken_pluspercent )
					) / 100.0f ) );
		}

		VIRTUAL_STAT( total_nonlethal_fire_damage_taken_per_minute,
			nonlethal_fire_damage_taken_per_minute,
			fire_damage_resistance_percent,
			degen_effect_pluspercent,
			damage_taken_pluspercent,
			virtual_elemental_damage_taken_pluspercent,
			fire_damage_taken_pluspercent,
			burning_damage_taken_pluspercent,
			cannot_be_damaged,
			fire_damage_immunity )
		{
			if ( stats.GetStat( cannot_be_damaged ) || stats.GetStat( fire_damage_immunity ) )
				return 0;

			const int damage_reduction_percent = stats.GetStat( fire_damage_resistance_percent );
			const int base_amount = static_cast< int >( stats.GetStat( nonlethal_fire_damage_taken_per_minute ) * ( 1.0f - damage_reduction_percent / 100.0f ) ); //integer math here causes early wraparound due to multiplying value by 100
			return static_cast< int >( base_amount * ( 1.0f + (
					stats.GetStat( degen_effect_pluspercent ) +
					stats.GetStat( damage_taken_pluspercent ) +
					stats.GetStat( virtual_elemental_damage_taken_pluspercent ) +
					stats.GetStat( fire_damage_taken_pluspercent ) +
					stats.GetStat( burning_damage_taken_pluspercent )
					) / 100.0f ) );
		}

		VIRTUAL_STAT( total_cold_damage_taken_per_minute,
			cold_damage_taken_per_minute,
			cold_damage_resistance_percent,
			degen_effect_pluspercent,
			damage_taken_pluspercent,
			cold_damage_taken_pluspercent,
			virtual_elemental_damage_taken_pluspercent,
			cannot_be_damaged,
			cold_damage_immunity )
		{
			if ( stats.GetStat( cannot_be_damaged ) || stats.GetStat( cold_damage_immunity ) )
				return 0;

			const int damage_reduction_percent = stats.GetStat( cold_damage_resistance_percent );
			const int base_amount = static_cast< int >( stats.GetStat( cold_damage_taken_per_minute ) * ( 1.0f - damage_reduction_percent / 100.0f ) ); //integer math here causes early wraparound due to multiplying value by 100
			return static_cast< int >( base_amount * ( 1.0f + (
				stats.GetStat( degen_effect_pluspercent ) +
				stats.GetStat( virtual_elemental_damage_taken_pluspercent ) +
				stats.GetStat( cold_damage_taken_pluspercent ) +
				stats.GetStat( damage_taken_pluspercent )
				) / 100.0f ) );
		}

		VIRTUAL_STAT( total_lightning_damage_taken_per_minute,
			lightning_damage_taken_per_minute,
			lightning_damage_resistance_percent,
			degen_effect_pluspercent,
			damage_taken_pluspercent,
			lightning_damage_taken_pluspercent,
			virtual_elemental_damage_taken_pluspercent,
			cannot_be_damaged,
			lightning_damage_immunity )
		{
			if ( stats.GetStat( cannot_be_damaged ) || stats.GetStat( lightning_damage_immunity ) )
				return 0;

			const int damage_reduction_percent = stats.GetStat( lightning_damage_resistance_percent );
			const int base_amount = static_cast< int >( stats.GetStat( lightning_damage_taken_per_minute ) * ( 1.0f - damage_reduction_percent / 100.0f ) ); //integer math here causes early wraparound due to multiplying value by 100
			return static_cast< int >( base_amount * ( 1.0f + (
				stats.GetStat( degen_effect_pluspercent ) +
				stats.GetStat( virtual_elemental_damage_taken_pluspercent ) +
				stats.GetStat( lightning_damage_taken_pluspercent ) +
				stats.GetStat( damage_taken_pluspercent )
				) / 100.0f ) );
		}

		VIRTUAL_STAT( total_chaos_damage_taken_per_minute,
			chaos_damage_taken_per_minute,
			chaos_damage_resistance_percent,
			chaos_damage_taken_pluspercent,
			degen_effect_pluspercent,
			damage_taken_pluspercent,
			cannot_be_damaged,
			chaos_damage_immunity,
			chaos_damage_taken_over_time_pluspercent )
		{
			if ( stats.GetStat( cannot_be_damaged ) || stats.GetStat( chaos_damage_immunity ) )
				return 0;

			const int damage_reduction_percent = stats.GetStat( chaos_damage_resistance_percent );
			const float base_amount = stats.GetStat( chaos_damage_taken_per_minute ) * ( 1.0f - damage_reduction_percent / 100.0f ); //integer math here causes early wraparound due to multiplying value by 100
			return static_cast< int >( base_amount * ( 1.0f + (
				stats.GetStat( degen_effect_pluspercent ) +
				stats.GetStat( damage_taken_pluspercent ) +
				stats.GetStat( chaos_damage_taken_pluspercent ) + 
				stats.GetStat( chaos_damage_taken_over_time_pluspercent )
				) / 100.0f ) );
		}

		VIRTUAL_STAT( total_damage_taken_per_minute_to_energy_shield,
			energy_shield_protects_mana,
			total_physical_damage_taken_per_minute,
			total_fire_damage_taken_per_minute, fire_damage_heals,
			total_cold_damage_taken_per_minute, cold_damage_heals,
			total_lightning_damage_taken_per_minute, lightning_damage_heals,
			total_chaos_damage_taken_per_minute, chaos_damage_does_not_bypass_energy_shield )
		{
			if ( stats.GetStat( energy_shield_protects_mana ) )
				return 0;

			return stats.GetStat( total_physical_damage_taken_per_minute ) +
				( stats.GetStat( fire_damage_heals ) ? 0 : stats.GetStat( total_fire_damage_taken_per_minute ) ) +
				( stats.GetStat( cold_damage_heals ) ? 0 : stats.GetStat( total_cold_damage_taken_per_minute ) ) +
				( stats.GetStat( lightning_damage_heals ) ? 0 : stats.GetStat( total_lightning_damage_taken_per_minute ) ) +
				( stats.GetStat( chaos_damage_does_not_bypass_energy_shield ) ? stats.GetStat( total_chaos_damage_taken_per_minute ) : 0 );
		}

		VIRTUAL_STAT( total_nonlethal_damage_taken_per_minute_to_energy_shield,
			energy_shield_protects_mana,
			total_nonlethal_fire_damage_taken_per_minute, fire_damage_heals )
		{
			if ( stats.GetStat( energy_shield_protects_mana ) )
				return 0;

			return ( stats.GetStat( fire_damage_heals ) ? 0 : stats.GetStat( total_nonlethal_fire_damage_taken_per_minute ) );
		}

		VIRTUAL_STAT( total_damage_taken_per_minute_to_life,
			energy_shield_protects_mana,
			total_physical_damage_taken_per_minute,
			total_fire_damage_taken_per_minute, fire_damage_heals,
			total_cold_damage_taken_per_minute, cold_damage_heals,
			total_lightning_damage_taken_per_minute, lightning_damage_heals,
			total_chaos_damage_taken_per_minute, chaos_damage_does_not_bypass_energy_shield )
		{
			if ( stats.GetStat( energy_shield_protects_mana ) )
			{
				return stats.GetStat( total_physical_damage_taken_per_minute ) +
					( stats.GetStat( fire_damage_heals ) ? 0 : stats.GetStat( total_fire_damage_taken_per_minute ) ) +
					( stats.GetStat( cold_damage_heals ) ? 0 : stats.GetStat( total_cold_damage_taken_per_minute ) ) +
					( stats.GetStat( lightning_damage_heals ) ? 0 : stats.GetStat( total_lightning_damage_taken_per_minute ) ) +
					stats.GetStat( total_chaos_damage_taken_per_minute );
			}
			else
			{
				return stats.GetStat( chaos_damage_does_not_bypass_energy_shield ) ? 0 : stats.GetStat( total_chaos_damage_taken_per_minute );
			}
		}

		VIRTUAL_STAT( total_nonlethal_damage_taken_per_minute_to_life,
			energy_shield_protects_mana,
			total_nonlethal_fire_damage_taken_per_minute, fire_damage_heals )
		{
			if ( !stats.GetStat( energy_shield_protects_mana ) )
				return 0;

			return stats.GetStat( fire_damage_heals ) ? 0 : stats.GetStat( total_nonlethal_fire_damage_taken_per_minute );
		}

		VIRTUAL_STAT( total_healing_from_damage_taken_per_minute,
			total_fire_damage_taken_per_minute, fire_damage_heals,
			total_nonlethal_fire_damage_taken_per_minute,
			total_cold_damage_taken_per_minute, cold_damage_heals,
			total_lightning_damage_taken_per_minute, lightning_damage_heals )
		{
			return ( stats.GetStat( fire_damage_heals ) ? stats.GetStat( total_fire_damage_taken_per_minute ) : 0 ) +
				( stats.GetStat( fire_damage_heals ) ? stats.GetStat( total_nonlethal_fire_damage_taken_per_minute ) : 0 ) +
				( stats.GetStat( cold_damage_heals ) ? stats.GetStat( total_cold_damage_taken_per_minute ) : 0 ) +
				( stats.GetStat( lightning_damage_heals ) ? stats.GetStat( total_lightning_damage_taken_per_minute ) : 0 );
		}

		/*
		 * Deal no damage stats
		 */

		VIRTUAL_STAT( deal_no_damage,
			deal_no_damage_yourself,
			damage_not_from_skill_user,
			base_deal_no_damage )
		{
			return stats.GetStat( base_deal_no_damage ) ||
				( stats.GetStat( deal_no_damage_yourself ) && !stats.GetStat( damage_not_from_skill_user ) );
		}

		VIRTUAL_STAT( deal_no_physical_damage,
			movement_skills_deal_no_physical_damage, skill_is_movement_skill,
			base_deal_no_physical_damage,
			deal_no_non_fire_damage )
		{
			return stats.GetStat( base_deal_no_physical_damage ) || stats.GetStat( deal_no_non_fire_damage ) ||
				( stats.GetStat( skill_is_movement_skill ) && stats.GetStat( movement_skills_deal_no_physical_damage ) );
		}

		VIRTUAL_STAT( deal_no_fire_damage,
			base_deal_no_fire_damage,
			deal_no_elemental_damage )
		{
			return stats.GetStat( base_deal_no_fire_damage ) || stats.GetStat( deal_no_elemental_damage );
		}

		VIRTUAL_STAT( deal_no_cold_damage,
			base_deal_no_cold_damage,
			deal_no_elemental_damage,
			deal_no_non_fire_damage )
		{
			return stats.GetStat( base_deal_no_cold_damage ) || stats.GetStat( deal_no_non_fire_damage ) || stats.GetStat( deal_no_elemental_damage );
		}

		VIRTUAL_STAT( deal_no_lightning_damage,
			base_deal_no_lightning_damage,
			deal_no_non_fire_damage,
			deal_no_elemental_damage )
		{
			return stats.GetStat( base_deal_no_lightning_damage ) || stats.GetStat( deal_no_non_fire_damage ) || stats.GetStat( deal_no_elemental_damage );
		}

		VIRTUAL_STAT( deal_no_chaos_damage,
			base_deal_no_chaos_damage,
			deal_no_non_fire_damage )
		{
			return stats.GetStat( base_deal_no_chaos_damage ) || stats.GetStat( deal_no_non_fire_damage );
		}

		VIRTUAL_STAT( deal_no_attack_damage,
			deal_no_damage,
			base_deal_no_attack_damage )
		{
			return stats.GetStat( deal_no_damage ) || stats.GetStat( base_deal_no_attack_damage );
		}

		VIRTUAL_STAT( deal_no_spell_damage,
			deal_no_damage,
			base_deal_no_spell_damage,
			local_can_only_deal_damage_with_this_weapon )
		{
			return stats.GetStat( deal_no_damage ) || stats.GetStat( base_deal_no_spell_damage ) || stats.GetStat( local_can_only_deal_damage_with_this_weapon );
		}

		VIRTUAL_STAT( deal_no_secondary_damage,
			deal_no_damage,
			base_deal_no_secondary_damage,
			local_can_only_deal_damage_with_this_weapon )
		{
			return stats.GetStat( deal_no_damage ) || stats.GetStat( base_deal_no_secondary_damage ) || stats.GetStat( local_can_only_deal_damage_with_this_weapon );
		}

		VIRTUAL_STAT( deal_no_main_hand_damage,
			deal_no_attack_damage,
			base_deal_no_main_hand_damage )
		{
			return stats.GetStat( deal_no_attack_damage ) || stats.GetStat( base_deal_no_main_hand_damage );
		}

		VIRTUAL_STAT( deal_no_off_hand_damage,
			deal_no_attack_damage,
			base_deal_no_off_hand_damage )
		{
			return stats.GetStat( deal_no_attack_damage ) || stats.GetStat( base_deal_no_off_hand_damage );
		}

		VIRTUAL_STAT( deal_no_damage_over_time,
			deal_no_damage,
			base_deal_no_damage_over_time,
			local_can_only_deal_damage_with_this_weapon,
			base_deal_no_spell_damage, spell_damage_modifiers_apply_to_damage_over_time )
		{
			return stats.GetStat( deal_no_damage ) || stats.GetStat( base_deal_no_damage_over_time ) || stats.GetStat( local_can_only_deal_damage_with_this_weapon ) ||
				( stats.GetStat( base_deal_no_spell_damage ) && stats.GetStat( spell_damage_modifiers_apply_to_damage_over_time ) );
		}

		VIRTUAL_STAT( deal_no_spell_physical_damage,
			deal_no_spell_damage,
			deal_no_physical_damage )
		{
			return stats.GetStat( deal_no_spell_damage ) || stats.GetStat( deal_no_physical_damage );
		}

		VIRTUAL_STAT( deal_no_spell_fire_damage,
			deal_no_spell_damage,
			deal_no_fire_damage )
		{
			return stats.GetStat( deal_no_spell_damage ) || stats.GetStat( deal_no_fire_damage );
		}

		VIRTUAL_STAT( deal_no_spell_cold_damage,
			deal_no_spell_damage,
			deal_no_cold_damage )
		{
			return stats.GetStat( deal_no_spell_damage ) || stats.GetStat( deal_no_cold_damage );
		}

		VIRTUAL_STAT( deal_no_spell_lightning_damage,
			deal_no_spell_damage,
			deal_no_lightning_damage )
		{
			return stats.GetStat( deal_no_spell_damage ) || stats.GetStat( deal_no_lightning_damage );
		}

		VIRTUAL_STAT( deal_no_spell_chaos_damage,
			deal_no_spell_damage,
			deal_no_chaos_damage )
		{
			return stats.GetStat( deal_no_spell_damage ) || stats.GetStat( deal_no_chaos_damage );
		}

		VIRTUAL_STAT( deal_no_secondary_physical_damage,
			deal_no_secondary_damage,
			deal_no_physical_damage )
		{
			return stats.GetStat( deal_no_secondary_damage ) || stats.GetStat( deal_no_physical_damage );
		}

		VIRTUAL_STAT( deal_no_secondary_fire_damage,
			deal_no_secondary_damage,
			deal_no_fire_damage )
		{
			return stats.GetStat( deal_no_secondary_damage ) || stats.GetStat( deal_no_fire_damage );
		}

		VIRTUAL_STAT( deal_no_secondary_cold_damage,
			deal_no_secondary_damage,
			deal_no_cold_damage )
		{
			return stats.GetStat( deal_no_secondary_damage ) || stats.GetStat( deal_no_cold_damage );
		}

		VIRTUAL_STAT( deal_no_secondary_lightning_damage,
			deal_no_secondary_damage,
			deal_no_lightning_damage )
		{
			return stats.GetStat( deal_no_secondary_damage ) || stats.GetStat( deal_no_lightning_damage );
		}

		VIRTUAL_STAT( deal_no_secondary_chaos_damage,
			deal_no_secondary_damage,
			deal_no_chaos_damage )
		{
			return stats.GetStat( deal_no_secondary_damage ) || stats.GetStat( deal_no_chaos_damage );
		}

		VIRTUAL_STAT( deal_no_main_hand_physical_damage,
			deal_no_main_hand_damage,
			deal_no_physical_damage,
			attacks_deal_no_physical_damage )
		{
			return stats.GetStat( deal_no_main_hand_damage ) || stats.GetStat( deal_no_physical_damage ) || stats.GetStat( attacks_deal_no_physical_damage );
		}

		VIRTUAL_STAT( deal_no_main_hand_fire_damage,
			deal_no_main_hand_damage,
			deal_no_fire_damage )
		{
			return stats.GetStat( deal_no_main_hand_damage ) || stats.GetStat( deal_no_fire_damage );
		}

		VIRTUAL_STAT( deal_no_main_hand_cold_damage,
			deal_no_main_hand_damage,
			deal_no_cold_damage )
		{
			return stats.GetStat( deal_no_main_hand_damage ) || stats.GetStat( deal_no_cold_damage );
		}

		VIRTUAL_STAT( deal_no_main_hand_lightning_damage,
			deal_no_main_hand_damage,
			deal_no_lightning_damage )
		{
			return stats.GetStat( deal_no_main_hand_damage ) || stats.GetStat( deal_no_lightning_damage );
		}

		VIRTUAL_STAT( deal_no_main_hand_chaos_damage,
			deal_no_main_hand_damage,
			deal_no_chaos_damage )
		{
			return stats.GetStat( deal_no_main_hand_damage ) || stats.GetStat( deal_no_chaos_damage );
		}

		VIRTUAL_STAT( deal_no_off_hand_physical_damage,
			deal_no_off_hand_damage,
			deal_no_physical_damage,
			attacks_deal_no_physical_damage )
		{
			return stats.GetStat( deal_no_off_hand_damage ) || stats.GetStat( deal_no_physical_damage ) || stats.GetStat( attacks_deal_no_physical_damage );
		}

		VIRTUAL_STAT( deal_no_off_hand_fire_damage,
			deal_no_off_hand_damage,
			deal_no_fire_damage )
		{
			return stats.GetStat( deal_no_off_hand_damage ) || stats.GetStat( deal_no_fire_damage );
		}

		VIRTUAL_STAT( deal_no_off_hand_cold_damage,
			deal_no_off_hand_damage,
			deal_no_cold_damage )
		{
			return stats.GetStat( deal_no_off_hand_damage ) || stats.GetStat( deal_no_cold_damage );
		}

		VIRTUAL_STAT( deal_no_off_hand_lightning_damage,
			deal_no_off_hand_damage,
			deal_no_lightning_damage )
		{
			return stats.GetStat( deal_no_off_hand_damage ) || stats.GetStat( deal_no_lightning_damage );
		}

		VIRTUAL_STAT( deal_no_off_hand_chaos_damage,
			deal_no_off_hand_damage,
			deal_no_chaos_damage )
		{
			return stats.GetStat( deal_no_off_hand_damage ) || stats.GetStat( deal_no_chaos_damage );
		}

		VIRTUAL_STAT( deal_no_physical_damage_over_time,
			deal_no_damage_over_time,
			deal_no_physical_damage )
		{
			return stats.GetStat( deal_no_damage_over_time ) || stats.GetStat( deal_no_physical_damage );
		}

		VIRTUAL_STAT( deal_no_fire_damage_over_time,
			deal_no_damage_over_time,
			deal_no_fire_damage )
		{
			return stats.GetStat( deal_no_damage_over_time ) || stats.GetStat( deal_no_fire_damage );
		}

		VIRTUAL_STAT( deal_no_cold_damage_over_time,
			deal_no_damage_over_time,
			deal_no_cold_damage )
		{
			return stats.GetStat( deal_no_damage_over_time ) || stats.GetStat( deal_no_cold_damage );
		}

		VIRTUAL_STAT( deal_no_lightning_damage_over_time,
			deal_no_damage_over_time,
			deal_no_lightning_damage )
		{
			return stats.GetStat( deal_no_damage_over_time ) || stats.GetStat( deal_no_lightning_damage );
		}

		VIRTUAL_STAT( deal_no_chaos_damage_over_time,
			deal_no_damage_over_time,
			deal_no_chaos_damage )
		{
			return stats.GetStat( deal_no_damage_over_time ) || stats.GetStat( deal_no_chaos_damage );
		}

		/*
		 * Damage related stats
		 */

		 //used by many of the damage increase stats to know when to apply projectile damage bonuses
		VIRTUAL_STAT( is_projectile,
			base_is_projectile,
			skill_can_fire_arrows,
			skill_can_fire_wand_projectiles,
			main_hand_weapon_type ) //since the off hand can only be a projectile weapon type if the main hand is as well, and can only be a non-projectile weapon type if the main hand is as well, we can just check main hand, and know that off hand matches
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( base_is_projectile ) ||
				stats.GetStat( skill_can_fire_arrows ) && main_hand_weapon_index == Items::Bow || //spell skills won't ever have this, so weapon type will never make them projectile damage when they shouldn't be
				stats.GetStat( skill_can_fire_wand_projectiles ) && main_hand_weapon_index == Items::Wand;
		}

		//used by attacks to know when to apply 'melee' bonuses
		VIRTUAL_STAT( attack_is_melee,
			is_projectile,
			main_hand_weapon_type,
			attack_is_not_melee_override,
			level ) //Horrible hackery. If a virtual stat should have a nonzero value when all it's stats are 0, it won't be assigned that value if non of them have ever changed.
					//This is the case for monsters which are set to 0 (OnehandSword) as base weapon type for this stat, such as Hillock. So we include a stat here that will be nonzero to force the calculation of this virtual stat.
		{
			return Items::IsMelee[stats.GetStat( main_hand_weapon_type )] && !stats.GetStat( is_projectile ) && !stats.GetStat( attack_is_not_melee_override );
		}

		/*

		END DAMAGE STATS

		*/


		VIRTUAL_STAT( stun_recovery_pluspercent,
			base_stun_recovery_pluspercent,
			stun_recovery_pluspercent_per_frenzy_charge,
			current_frenzy_charges )
		{
			return stats.GetStat( base_stun_recovery_pluspercent ) + ( stats.GetStat( stun_recovery_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) );
		}

		VIRTUAL_STAT( main_hand_reduce_enemy_block_percent,
			global_reduce_enemy_block_percent,
			while_using_sword_reduce_enemy_block_percent, main_hand_weapon_type,
			bow_enemy_block_minuspercent,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const unsigned main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const bool is_using_sword = main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::OneHandSword || main_hand_weapon_index == Items::OneHandSwordThrusting || main_hand_weapon_index == Items::TwoHandSword;
			const bool is_using_bow = main_hand_weapon_index == Items::Bow;
			return ( is_using_sword ? stats.GetStat( while_using_sword_reduce_enemy_block_percent ) : 0 ) +
				( is_using_bow ? stats.GetStat( bow_enemy_block_minuspercent ) : 0 ) +
				stats.GetStat( global_reduce_enemy_block_percent );
		}

		VIRTUAL_STAT( off_hand_reduce_enemy_block_percent,
			global_reduce_enemy_block_percent,
			while_using_sword_reduce_enemy_block_percent, off_hand_weapon_type,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool is_using_sword = off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::OneHandSword || off_hand_weapon_index == Items::OneHandSwordThrusting || off_hand_weapon_index == Items::TwoHandSword;
			return ( is_using_sword ? stats.GetStat( while_using_sword_reduce_enemy_block_percent ) : 0 ) +
				stats.GetStat( global_reduce_enemy_block_percent );
		}

		VIRTUAL_STAT( spell_critical_strike_multiplier_plus,
			base_critical_strike_multiplier_plus,
			base_spell_critical_strike_multiplier_plus,
			global_critical_strike_multiplier_plus_while_holding_staff, main_hand_weapon_type, global_critical_strike_multiplier_plus_while_holding_bow,
			critical_strike_multiplier_is_100,
			trap_critical_strike_multiplier_plus, skill_is_trapped,
			mine_critical_strike_multiplier_plus, skill_is_mined,
			fire_critical_strike_multiplier_plus, skill_is_fire_skill,
			lightning_critical_strike_multiplier_plus, skill_is_lightning_skill,
			cold_critical_strike_multiplier_plus, skill_is_cold_skill,
			chaos_critical_strike_multiplier_plus, skill_is_chaos_skill,
			elemental_critical_strike_multiplier_plus,
			vaal_skill_critical_strike_multiplier_plus, skill_is_vaal_skill,
			totem_critical_strike_multiplier_plus, skill_is_totemified,
			global_critical_strike_multiplier_while_dual_wielding_plus, is_dual_wielding,
			critical_strike_multiplier_plus_per_power_charge, current_power_charges )
		{
			if ( stats.GetStat( critical_strike_multiplier_is_100 ) )
				return 100;

			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			unsigned spell_critical_strike_increase = 0;
			spell_critical_strike_increase += stats.GetStat( base_critical_strike_multiplier_plus ) + stats.GetStat( base_spell_critical_strike_multiplier_plus ) +
				( main_hand_weapon_index == Items::Staff ? stats.GetStat( global_critical_strike_multiplier_plus_while_holding_staff ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( global_critical_strike_multiplier_plus_while_holding_bow ) : 0 );

			spell_critical_strike_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_multiplier_plus ) : 0;
			spell_critical_strike_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_multiplier_plus ) : 0;
			spell_critical_strike_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_multiplier_plus ) : 0;

			spell_critical_strike_increase += stats.GetStat( critical_strike_multiplier_plus_per_power_charge ) * stats.GetStat( current_power_charges );

			// Dual wielding
			spell_critical_strike_increase += stats.GetStat( is_dual_wielding ) ? stats.GetStat( global_critical_strike_multiplier_while_dual_wielding_plus ) : 0;

			bool is_elemental_skill = false;

			if ( stats.GetStat( skill_is_fire_skill ) > 0 )
			{
				is_elemental_skill = true;
				spell_critical_strike_increase += stats.GetStat( fire_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_cold_skill ) > 0 )
			{
				is_elemental_skill = true;
				spell_critical_strike_increase += stats.GetStat( cold_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_lightning_skill ) > 0 )
			{
				is_elemental_skill = true;
				spell_critical_strike_increase += stats.GetStat( lightning_critical_strike_multiplier_plus );
			}

			spell_critical_strike_increase += ( is_elemental_skill ? stats.GetStat( elemental_critical_strike_multiplier_plus ) : 0 );

			spell_critical_strike_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_multiplier_plus ) : 0;
			spell_critical_strike_increase += stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_multiplier_plus ) : 0;

			return spell_critical_strike_increase;
		}

		VIRTUAL_STAT( secondary_damage_critical_strike_multiplier_plus,
			base_critical_strike_multiplier_plus,
			global_critical_strike_multiplier_plus_while_holding_staff, main_hand_weapon_type, global_critical_strike_multiplier_plus_while_holding_bow,
			critical_strike_multiplier_is_100,
			trap_critical_strike_multiplier_plus, skill_is_trapped,
			mine_critical_strike_multiplier_plus, skill_is_mined,
			fire_critical_strike_multiplier_plus, skill_is_fire_skill,
			lightning_critical_strike_multiplier_plus, skill_is_lightning_skill,
			cold_critical_strike_multiplier_plus, skill_is_cold_skill,
			chaos_critical_strike_multiplier_plus, skill_is_chaos_skill,
			elemental_critical_strike_multiplier_plus,
			vaal_skill_critical_strike_multiplier_plus, skill_is_vaal_skill,
			totem_critical_strike_multiplier_plus, skill_is_totemified,
			critical_strike_multiplier_plus_per_power_charge, current_power_charges,
			global_critical_strike_multiplier_while_dual_wielding_plus, is_dual_wielding )
		{
			if ( stats.GetStat( critical_strike_multiplier_is_100 ) )
				return 100;

			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			unsigned secondary_critical_multiplier_increase = 0;
			secondary_critical_multiplier_increase += stats.GetStat( base_critical_strike_multiplier_plus ) +
				( main_hand_weapon_index == Items::Staff ? stats.GetStat( global_critical_strike_multiplier_plus_while_holding_staff ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( global_critical_strike_multiplier_plus_while_holding_bow ) : 0 );

			secondary_critical_multiplier_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_multiplier_plus ) : 0;
			secondary_critical_multiplier_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_multiplier_plus ) : 0;
			secondary_critical_multiplier_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_multiplier_plus ) : 0;

			secondary_critical_multiplier_increase += stats.GetStat( critical_strike_multiplier_plus_per_power_charge ) * stats.GetStat( current_power_charges );
			secondary_critical_multiplier_increase += stats.GetStat( is_dual_wielding ) ? stats.GetStat( global_critical_strike_multiplier_while_dual_wielding_plus ) : 0;

			bool is_elemental_skill = false;

			if ( stats.GetStat( skill_is_fire_skill ) > 0 )
			{
				is_elemental_skill = true;
				secondary_critical_multiplier_increase += stats.GetStat( fire_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_cold_skill ) > 0 )
			{
				is_elemental_skill = true;
				secondary_critical_multiplier_increase += stats.GetStat( cold_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_lightning_skill ) > 0 )
			{
				is_elemental_skill = true;
				secondary_critical_multiplier_increase += stats.GetStat( lightning_critical_strike_multiplier_plus );
			}

			secondary_critical_multiplier_increase += ( is_elemental_skill ? stats.GetStat( elemental_critical_strike_multiplier_plus ) : 0 );

			secondary_critical_multiplier_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_multiplier_plus ) : 0;
			secondary_critical_multiplier_increase += stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_multiplier_plus ) : 0;

			return secondary_critical_multiplier_increase;
		}

		Loaders::StatsValues::Stats critical_multiplier_stats[Items::NumWeaponClasses + 1] =
		{
			sword_critical_strike_multiplier_plus,				//OneHandSword,
			sword_critical_strike_multiplier_plus,				//OneHandSwordThrusting,
			sword_critical_strike_multiplier_plus,				//TwoHandSword,
			mace_critical_strike_multiplier_plus,				//OneHandMace,
			mace_critical_strike_multiplier_plus,				//TwoHandMace,
			mace_critical_strike_multiplier_plus,				//Sceptre,
			wand_critical_strike_multiplier_plus,				//Wand,
			axe_critical_strike_multiplier_plus,					//OneHandAxe,
			axe_critical_strike_multiplier_plus,					//TwoHandAxe,
			bow_critical_strike_multiplier_plus,					//Bow,
			critical_strike_multiplier_with_dagger_plus,			//Dagger,
			staff_critical_strike_multiplier_plus,				//Staff,
			claw_critical_strike_multiplier_plus,				//Claw,
			Loaders::StatsValues::Stats( 0 ),							//Shield,
			Loaders::StatsValues::Stats( 0 ),							//Unarmed,
			Loaders::StatsValues::Stats( 0 ),							//FishingRod,
			Loaders::StatsValues::Stats( 0 ),							//NumWeaponClasses
		};

		Loaders::StatsValues::Stats varunastra_critical_multiplier_stats[Items::NumWeaponClasses + 1] =
		{
			sword_critical_strike_multiplier_plus,				//OneHandSword,
			mace_critical_strike_multiplier_plus,				//OneHandMace,
			axe_critical_strike_multiplier_plus,				//OneHandAxe,
			critical_strike_multiplier_with_dagger_plus,		//Dagger,
			claw_critical_strike_multiplier_plus,				//Claw,
		};

		VIRTUAL_STAT( main_hand_critical_strike_multiplier_plus,
			base_critical_strike_multiplier_plus,
			main_hand_local_critical_strike_multiplier_plus,
			main_hand_weapon_type,
			one_handed_melee_critical_strike_multiplier_plus,
			two_handed_melee_critical_strike_multiplier_plus,
			critical_strike_multiplier_while_dual_wielding_plus, is_dual_wielding,
			global_critical_strike_multiplier_while_dual_wielding_plus, 
			sword_critical_strike_multiplier_plus,
			mace_critical_strike_multiplier_plus,
			axe_critical_strike_multiplier_plus,
			bow_critical_strike_multiplier_plus,
			wand_critical_strike_multiplier_plus,
			claw_critical_strike_multiplier_plus,
			staff_critical_strike_multiplier_plus,
			critical_strike_multiplier_with_dagger_plus,
			global_critical_strike_multiplier_plus_while_holding_staff,
			global_critical_strike_multiplier_plus_while_holding_bow,
			melee_weapon_critical_strike_multiplier_plus, attack_is_melee,
			critical_strike_multiplier_is_100,
			melee_critical_strike_multiplier_plus_while_wielding_shield, off_hand_weapon_type,
			trap_critical_strike_multiplier_plus, skill_is_trapped,
			mine_critical_strike_multiplier_plus, skill_is_mined,
			fire_critical_strike_multiplier_plus, skill_is_fire_skill,
			lightning_critical_strike_multiplier_plus, skill_is_lightning_skill,
			cold_critical_strike_multiplier_plus, skill_is_cold_skill,
			chaos_critical_strike_multiplier_plus, skill_is_chaos_skill,
			elemental_critical_strike_multiplier_plus,
			vaal_skill_critical_strike_multiplier_plus, skill_is_vaal_skill,
			totem_critical_strike_multiplier_plus, skill_is_totemified,
			critical_strike_multiplier_plus_per_power_charge, current_power_charges,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			if ( stats.GetStat( critical_strike_multiplier_is_100 ) )
				return 100;

			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			unsigned weapon_specific_critical_multiplier_increase = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_critical_multiplier_stats ), std::end( varunastra_critical_multiplier_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_critical_multiplier_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = critical_multiplier_stats[main_hand_weapon_index] )
					weapon_specific_critical_multiplier_increase += stats.GetStat( stat );
			}

			//melee attacks
			if ( stats.GetStat( attack_is_melee ) )
			{
				weapon_specific_critical_multiplier_increase += stats.GetStat( melee_weapon_critical_strike_multiplier_plus );
				weapon_specific_critical_multiplier_increase += ( main_hand_weapon_index == Items::Shield ? ( stats.GetStat( melee_critical_strike_multiplier_plus_while_wielding_shield ) ) : 0 );
			}

			//attacks with melee weapons
			if ( Items::IsMelee[size_t( stats.GetStat( main_hand_weapon_type ) )] && Items::IsWeapon[size_t( stats.GetStat( main_hand_weapon_type ) )] )
			{
				if ( Items::IsTwoHanded[size_t( stats.GetStat( main_hand_weapon_type ) )] )
					weapon_specific_critical_multiplier_increase += stats.GetStat( two_handed_melee_critical_strike_multiplier_plus );
				else if ( Items::IsOneHanded[size_t( stats.GetStat( main_hand_weapon_type ) )] )
					weapon_specific_critical_multiplier_increase += stats.GetStat( one_handed_melee_critical_strike_multiplier_plus );
			}

			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_multiplier_plus ) : 0;
			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_multiplier_plus ) : 0;
			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_multiplier_plus ) : 0;

			bool is_elemental_skill = false;

			if ( stats.GetStat( skill_is_fire_skill ) > 0 )
			{
				is_elemental_skill = true;
				weapon_specific_critical_multiplier_increase += stats.GetStat( fire_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_cold_skill ) > 0 )
			{
				is_elemental_skill = true;
				weapon_specific_critical_multiplier_increase += stats.GetStat( cold_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_lightning_skill ) > 0 )
			{
				is_elemental_skill = true;
				weapon_specific_critical_multiplier_increase += stats.GetStat( lightning_critical_strike_multiplier_plus );
			}

			weapon_specific_critical_multiplier_increase += ( is_elemental_skill ? stats.GetStat( elemental_critical_strike_multiplier_plus ) : 0 );

			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_multiplier_plus ) : 0;
			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_multiplier_plus ) : 0;

			weapon_specific_critical_multiplier_increase += stats.GetStat( critical_strike_multiplier_plus_per_power_charge ) * stats.GetStat( current_power_charges );

			if( stats.GetStat( is_dual_wielding ) )
			{
				weapon_specific_critical_multiplier_increase += stats.GetStat( critical_strike_multiplier_while_dual_wielding_plus );
				weapon_specific_critical_multiplier_increase += stats.GetStat( global_critical_strike_multiplier_while_dual_wielding_plus );
			}

			if ( main_hand_weapon_index == Items::Staff )
				weapon_specific_critical_multiplier_increase += stats.GetStat( global_critical_strike_multiplier_plus_while_holding_staff );
			else if ( main_hand_weapon_index == Items::Bow )
				weapon_specific_critical_multiplier_increase += stats.GetStat( global_critical_strike_multiplier_plus_while_holding_bow );

			return stats.GetStat( base_critical_strike_multiplier_plus ) + weapon_specific_critical_multiplier_increase + stats.GetStat( main_hand_local_critical_strike_multiplier_plus );
		}

		VIRTUAL_STAT( off_hand_critical_strike_multiplier_plus,
			base_critical_strike_multiplier_plus,
			off_hand_local_critical_strike_multiplier_plus,
			off_hand_weapon_type,
			one_handed_melee_critical_strike_multiplier_plus,
			two_handed_melee_critical_strike_multiplier_plus,
			critical_strike_multiplier_while_dual_wielding_plus, is_dual_wielding,
			global_critical_strike_multiplier_while_dual_wielding_plus,
			sword_critical_strike_multiplier_plus,
			mace_critical_strike_multiplier_plus,
			axe_critical_strike_multiplier_plus,
			claw_critical_strike_multiplier_plus,
			staff_critical_strike_multiplier_plus,
			critical_strike_multiplier_with_dagger_plus,
			melee_weapon_critical_strike_multiplier_plus,
			critical_strike_multiplier_is_100,
			trap_critical_strike_multiplier_plus, skill_is_trapped,
			mine_critical_strike_multiplier_plus, skill_is_mined,
			fire_critical_strike_multiplier_plus, skill_is_fire_skill,
			lightning_critical_strike_multiplier_plus, skill_is_lightning_skill,
			cold_critical_strike_multiplier_plus, skill_is_cold_skill,
			chaos_critical_strike_multiplier_plus, skill_is_chaos_skill,
			elemental_critical_strike_multiplier_plus,
			vaal_skill_critical_strike_multiplier_plus, skill_is_vaal_skill,
			totem_critical_strike_multiplier_plus, skill_is_totemified,
			critical_strike_multiplier_plus_per_power_charge, current_power_charges,
			attack_is_melee,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			if ( stats.GetStat( critical_strike_multiplier_is_100 ) )
				return 100;

			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			unsigned weapon_specific_critical_multiplier_increase = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_critical_multiplier_stats ), std::end( varunastra_critical_multiplier_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					weapon_specific_critical_multiplier_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( const auto stat = critical_multiplier_stats[off_hand_weapon_index] )
					weapon_specific_critical_multiplier_increase += stats.GetStat( stat );
			}

			//melee attacks
			if ( stats.GetStat( attack_is_melee ) )
			{
				weapon_specific_critical_multiplier_increase += stats.GetStat( melee_weapon_critical_strike_multiplier_plus );
			}

			//attacks with melee weapons
			if ( Items::IsMelee[size_t( stats.GetStat( off_hand_weapon_type ) )] && Items::IsWeapon[size_t( stats.GetStat( off_hand_weapon_type ) )] )
			{
				if ( Items::IsTwoHanded[size_t( stats.GetStat( off_hand_weapon_type ) )] )
					weapon_specific_critical_multiplier_increase += stats.GetStat( two_handed_melee_critical_strike_multiplier_plus );
				else if ( Items::IsOneHanded[size_t( stats.GetStat( off_hand_weapon_type ) )] )
					weapon_specific_critical_multiplier_increase += stats.GetStat( one_handed_melee_critical_strike_multiplier_plus );
			}

			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_multiplier_plus ) : 0;
			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_multiplier_plus ) : 0;
			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_multiplier_plus ) : 0;

			bool is_elemental_skill = false;

			if ( stats.GetStat( skill_is_fire_skill ) > 0 )
			{
				is_elemental_skill = true;
				weapon_specific_critical_multiplier_increase += stats.GetStat( fire_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_cold_skill ) > 0 )
			{
				is_elemental_skill = true;
				weapon_specific_critical_multiplier_increase += stats.GetStat( cold_critical_strike_multiplier_plus );
			}

			if ( stats.GetStat( skill_is_lightning_skill ) > 0 )
			{
				is_elemental_skill = true;
				weapon_specific_critical_multiplier_increase += stats.GetStat( lightning_critical_strike_multiplier_plus );
			}

			weapon_specific_critical_multiplier_increase += ( is_elemental_skill ? stats.GetStat( elemental_critical_strike_multiplier_plus ) : 0 );

			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_multiplier_plus ) : 0;
			weapon_specific_critical_multiplier_increase += stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_multiplier_plus ) : 0;

			weapon_specific_critical_multiplier_increase += stats.GetStat( critical_strike_multiplier_plus_per_power_charge ) * stats.GetStat( current_power_charges );

			if( stats.GetStat( is_dual_wielding ) )
			{
				weapon_specific_critical_multiplier_increase += stats.GetStat( critical_strike_multiplier_while_dual_wielding_plus );
				weapon_specific_critical_multiplier_increase += stats.GetStat( global_critical_strike_multiplier_while_dual_wielding_plus );
			}

			return stats.GetStat( base_critical_strike_multiplier_plus ) + weapon_specific_critical_multiplier_increase + stats.GetStat( off_hand_local_critical_strike_multiplier_plus );
		}

		VIRTUAL_STAT( self_critical_strike_multiplier_minuspercent,
			base_self_critical_strike_multiplier_minuspercent,
			self_critical_strike_multiplier_minuspercent_per_endurance_charge, current_endurance_charges )
		{
			return stats.GetStat( base_self_critical_strike_multiplier_minuspercent ) + ( stats.GetStat( self_critical_strike_multiplier_minuspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) );
		}

		const Loaders::StatsValues::Stats weapon_knockback_stats[Items::NumWeaponClasses + 1] =
		{
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			knockback_with_wand,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			knockback_with_bow,
			Loaders::StatsValues::Stats( 0 ),
			knockback_with_staff,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};

		VIRTUAL_STAT( main_hand_knockback,
			global_knockback,
			main_hand_local_knockback,
			melee_knockback, attack_is_melee,
			knockback_with_wand,
			knockback_with_bow,
			knockback_with_staff,
			cannot_knockback, main_hand_weapon_type,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			unsigned main_hand_weapon_knockback = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				// Currently no weapon types with knockback supported by varunastra.
			}
			else
			{
				if ( auto stat = weapon_knockback_stats[main_hand_weapon_index] )
					main_hand_weapon_knockback = stats.GetStat( stat );
			}

			return stats.GetStat( global_knockback ) || main_hand_weapon_knockback ||
				stats.GetStat( main_hand_local_knockback ) ||
				( stats.GetStat( melee_knockback ) && stats.GetStat( attack_is_melee ) );
		}

		VIRTUAL_STAT( off_hand_knockback,
			global_knockback,
			off_hand_local_knockback,
			melee_knockback, attack_is_melee,
			knockback_with_wand,
			knockback_with_bow,
			knockback_with_staff,
			cannot_knockback,
			off_hand_weapon_type,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			unsigned off_hand_weapon_knockback = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				// Currently no weapon types with knockback supported by varunastra.
			}
			else
			{
				if ( auto stat = weapon_knockback_stats[off_hand_weapon_index] )
					off_hand_weapon_knockback = stats.GetStat( stat );
			}

			return stats.GetStat( global_knockback ) || off_hand_weapon_knockback ||
				stats.GetStat( off_hand_local_knockback ) ||
				( stats.GetStat( melee_knockback ) && stats.GetStat( attack_is_melee ) );
		}

		const Loaders::StatsValues::Stats weapon_knockback_on_crit_stats[Items::NumWeaponClasses + 1] =
		{
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			knockback_on_crit_with_wand,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			knockback_on_crit_with_bow,
			Loaders::StatsValues::Stats( 0 ),
			knockback_on_crit_with_staff,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};

		VIRTUAL_STAT( main_hand_knockback_on_crit,
			global_knockback_on_crit,
			knockback_on_crit_with_wand, main_hand_weapon_type,
			knockback_on_crit_with_bow,
			knockback_on_crit_with_staff,
			cannot_knockback,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			unsigned main_hand_weapon_knockback_on_crit = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				// Currently no weapon types with knockback supported by varunastra.
			}
			else
			{
				if ( auto stat = weapon_knockback_on_crit_stats[main_hand_weapon_index] )
					main_hand_weapon_knockback_on_crit = stats.GetStat( stat );
			}

			return stats.GetStat( global_knockback_on_crit ) || main_hand_weapon_knockback_on_crit;
		}

		VIRTUAL_STAT( off_hand_knockback_on_crit,
			global_knockback_on_crit,
			knockback_on_crit_with_wand, off_hand_weapon_type,
			knockback_on_crit_with_bow,
			knockback_on_crit_with_staff,
			cannot_knockback,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			unsigned off_hand_weapon_knockback_on_crit = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				// Currently no weapon types with knockback supported by varunastra.
			}
			else
			{
				if ( auto stat = weapon_knockback_on_crit_stats[off_hand_weapon_index] )
					off_hand_weapon_knockback_on_crit = stats.GetStat( stat );
			}

			return stats.GetStat( global_knockback_on_crit ) || off_hand_weapon_knockback_on_crit;
		}

		VIRTUAL_STAT( main_hand_chance_to_knockback_percent,
			main_hand_knockback,
			global_chance_to_knockback_percent,
			cannot_knockback )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			if ( stats.GetStat( main_hand_knockback ) )
				return 100;

			return stats.GetStat( global_chance_to_knockback_percent );
		}

		VIRTUAL_STAT( off_hand_chance_to_knockback_percent,
			off_hand_knockback,
			global_chance_to_knockback_percent,
			cannot_knockback )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			if ( stats.GetStat( off_hand_knockback ) )
				return 100;

			return stats.GetStat( global_chance_to_knockback_percent );
		}

		VIRTUAL_STAT( global_chance_to_knockback_percent,
			global_knockback,
			base_global_chance_to_knockback_percent,
			knockback_on_counterattack_percent, is_counterattack,
			cannot_knockback )
		{
			if ( stats.GetStat( cannot_knockback ) )
				return 0;

			if ( stats.GetStat( global_knockback ) )
				return 100;

			return stats.GetStat( base_global_chance_to_knockback_percent ) +
				( stats.GetStat( is_counterattack ) ? stats.GetStat( knockback_on_counterattack_percent ) : 0 );
		}

		Loaders::StatsValues::Stats weapon_accuracy_rating_percentage_stats[ ] =
		{
			sword_accuracy_rating_pluspercent
			, sword_accuracy_rating_pluspercent
			, sword_accuracy_rating_pluspercent
			, mace_accuracy_rating_pluspercent
			, mace_accuracy_rating_pluspercent
			, mace_accuracy_rating_pluspercent
			, wand_accuracy_rating_pluspercent
			, axe_accuracy_rating_pluspercent
			, axe_accuracy_rating_pluspercent
			, bow_accuracy_rating_pluspercent
			, dagger_accuracy_rating_pluspercent
			, staff_accuracy_rating_pluspercent
			, claw_accuracy_rating_pluspercent
			, Loaders::StatsValues::Stats( 0 )
			, Loaders::StatsValues::Stats( 0 )
			, Loaders::StatsValues::Stats( 0 )
		};

		Loaders::StatsValues::Stats weapon_accuracy_rating_stats[ ] =
		{
			sword_accuracy_rating
			, sword_accuracy_rating
			, sword_accuracy_rating
			, mace_accuracy_rating
			, mace_accuracy_rating
			, mace_accuracy_rating
			, wand_accuracy_rating
			, axe_accuracy_rating
			, axe_accuracy_rating
			, bow_accuracy_rating
			, dagger_accuracy_rating
			, staff_accuracy_rating
			, claw_accuracy_rating
			, Loaders::StatsValues::Stats( 0 )
			, Loaders::StatsValues::Stats( 0 )
			, Loaders::StatsValues::Stats( 0 )
		};

		Loaders::StatsValues::Stats varunastra_accuracy_rating_percentage_stats[ ] =
		{
			sword_accuracy_rating_pluspercent
			, mace_accuracy_rating_pluspercent
			, axe_accuracy_rating_pluspercent
			, dagger_accuracy_rating_pluspercent
			, claw_accuracy_rating_pluspercent
		};

		Loaders::StatsValues::Stats varunastra_accuracy_rating_stats[ ] =
		{
			sword_accuracy_rating
			, mace_accuracy_rating
			, axe_accuracy_rating
			, dagger_accuracy_rating
			, claw_accuracy_rating
		};


		VIRTUAL_STAT( main_hand_accuracy_rating,
			accuracy_rating,
			accuracy_rating_pluspercent,
			accuracy_rating_while_dual_wielding_pluspercent, is_dual_wielding,
			main_hand_weapon_type,
			two_handed_melee_accuracy_rating_pluspercent,
			one_handed_melee_accuracy_rating_pluspercent,
			accuracy_rating_per_level, level,
			main_hand_local_accuracy_rating,
			dexterity,
			main_hand_local_accuracy_rating_pluspercent,
			sword_accuracy_rating_pluspercent,
			mace_accuracy_rating_pluspercent,
			wand_accuracy_rating_pluspercent,
			axe_accuracy_rating_pluspercent,
			bow_accuracy_rating_pluspercent,
			dagger_accuracy_rating_pluspercent,
			staff_accuracy_rating_pluspercent,
			claw_accuracy_rating_pluspercent,
			sword_accuracy_rating,
			mace_accuracy_rating,
			wand_accuracy_rating,
			axe_accuracy_rating,
			bow_accuracy_rating,
			dagger_accuracy_rating,
			staff_accuracy_rating,
			claw_accuracy_rating,
			accuracy_rating_pluspercent_per_frenzy_charge, current_frenzy_charges,
			accuracy_rating_pluspercent_when_on_low_life, on_low_life,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			X_accuracy_per_2_intelligence, intelligence )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_is_dual_wielding = !!stats.GetStat( is_dual_wielding );

			unsigned main_weapon_accuracy_rating = 0;
			unsigned main_weapon_accuracy_rating_percentage = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_accuracy_rating_stats ), std::end( varunastra_accuracy_rating_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					main_weapon_accuracy_rating += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = weapon_accuracy_rating_stats[main_hand_weapon_index] )
					main_weapon_accuracy_rating = stats.GetStat( stat );
			}

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_accuracy_rating_percentage_stats ), std::end( varunastra_accuracy_rating_percentage_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					main_weapon_accuracy_rating_percentage += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = weapon_accuracy_rating_percentage_stats[main_hand_weapon_index] )
					main_weapon_accuracy_rating_percentage = stats.GetStat( stat );
			}

			const float accuracy_from_dexterity = stats.GetStat( dexterity ) * 2.0f; //1 point of dex = 2 points accuracy

			return std::max( Round( ( main_weapon_accuracy_rating + stats.GetStat( accuracy_rating ) + ( stats.GetStat( accuracy_rating_per_level ) * ( stats.GetStat( level ) - 1 ) ) +
				stats.GetStat( main_hand_local_accuracy_rating ) + accuracy_from_dexterity + ( stats.GetStat( X_accuracy_per_2_intelligence ) * int( stats.GetStat( intelligence ) / 2 ) ) )
				* Scale(
					100 +
					stats.GetStat( accuracy_rating_pluspercent ) +
					( stats.GetStat( accuracy_rating_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
					( stats.GetStat( on_low_life ) ? stats.GetStat( accuracy_rating_pluspercent_when_on_low_life ) : 0 ) +
					( is_is_dual_wielding ? stats.GetStat( accuracy_rating_while_dual_wielding_pluspercent ) : 0 ) +
					main_weapon_accuracy_rating_percentage +
					( ( Items::IsTwoHanded[size_t( main_hand_weapon_index )] && Items::IsMelee[size_t( main_hand_weapon_index )] ) ? stats.GetStat( two_handed_melee_accuracy_rating_pluspercent ) : 0 ) +
					( ( Items::IsOneHanded[size_t( main_hand_weapon_index )] && Items::IsMelee[size_t( main_hand_weapon_index )] ) ? stats.GetStat( one_handed_melee_accuracy_rating_pluspercent ) : 0 ) ) *
				Scale( 100 + stats.GetStat( main_hand_local_accuracy_rating_pluspercent ) )
				), 0 );
		}

		VIRTUAL_STAT( off_hand_accuracy_rating,
			accuracy_rating,
			accuracy_rating_pluspercent,
			accuracy_rating_while_dual_wielding_pluspercent, is_dual_wielding,
			off_hand_weapon_type,
			two_handed_melee_accuracy_rating_pluspercent,
			one_handed_melee_accuracy_rating_pluspercent,
			accuracy_rating_per_level, level,
			off_hand_local_accuracy_rating,
			dexterity,
			off_hand_local_accuracy_rating_pluspercent,
			sword_accuracy_rating_pluspercent,
			mace_accuracy_rating_pluspercent,
			wand_accuracy_rating_pluspercent,
			axe_accuracy_rating_pluspercent,
			bow_accuracy_rating_pluspercent,
			dagger_accuracy_rating_pluspercent,
			staff_accuracy_rating_pluspercent,
			claw_accuracy_rating_pluspercent,
			sword_accuracy_rating,
			mace_accuracy_rating,
			wand_accuracy_rating,
			axe_accuracy_rating,
			bow_accuracy_rating,
			dagger_accuracy_rating,
			staff_accuracy_rating,
			claw_accuracy_rating,
			accuracy_rating_pluspercent_per_frenzy_charge, current_frenzy_charges,
			accuracy_rating_pluspercent_when_on_low_life, on_low_life,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			const bool is_is_dual_wielding = !!stats.GetStat( is_dual_wielding );

			unsigned off_weapon_accuracy_rating = 0;
			unsigned off_weapon_accuracy_rating_percentage = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_accuracy_rating_stats ), std::end( varunastra_accuracy_rating_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					off_weapon_accuracy_rating += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = weapon_accuracy_rating_stats[off_hand_weapon_index] )
					off_weapon_accuracy_rating = stats.GetStat( stat );
			}

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_accuracy_rating_percentage_stats ), std::end( varunastra_accuracy_rating_percentage_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					off_weapon_accuracy_rating_percentage += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = weapon_accuracy_rating_percentage_stats[off_hand_weapon_index] )
					off_weapon_accuracy_rating_percentage = stats.GetStat( stat );
			}

			const float accuracy_from_dexterity = stats.GetStat( dexterity ) * 2.0f; //1 point of dex = 2 points accuracy

			return std::max( Round( ( off_weapon_accuracy_rating + stats.GetStat( accuracy_rating ) + ( stats.GetStat( accuracy_rating_per_level ) * ( stats.GetStat( level ) - 1 ) ) +
				stats.GetStat( off_hand_local_accuracy_rating ) + accuracy_from_dexterity ) * Scale(
					100 +
					stats.GetStat( accuracy_rating_pluspercent ) +
					( stats.GetStat( accuracy_rating_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
					( stats.GetStat( on_low_life ) ? stats.GetStat( accuracy_rating_pluspercent_when_on_low_life ) : 0 ) +
					( is_is_dual_wielding ? stats.GetStat( accuracy_rating_while_dual_wielding_pluspercent ) : 0 ) +
					off_weapon_accuracy_rating_percentage +
					( ( Items::IsTwoHanded[size_t( off_hand_weapon_index )] && Items::IsMelee[size_t( off_hand_weapon_index )] ) ? stats.GetStat( two_handed_melee_accuracy_rating_pluspercent ) : 0 ) +
					( ( Items::IsOneHanded[size_t( off_hand_weapon_index )] && Items::IsMelee[size_t( off_hand_weapon_index )] ) ? stats.GetStat( one_handed_melee_accuracy_rating_pluspercent ) : 0 ) ) *
				Scale( 100 + stats.GetStat( off_hand_local_accuracy_rating_pluspercent ) )
				), 0 );
		}

		Loaders::StatsValues::Stats weapon_attack_speed_stats[ ] =
		{
			sword_attack_speed_pluspercent,
			sword_attack_speed_pluspercent,
			sword_attack_speed_pluspercent,
			mace_attack_speed_pluspercent,
			mace_attack_speed_pluspercent,
			mace_attack_speed_pluspercent,
			wand_attack_speed_pluspercent,
			axe_attack_speed_pluspercent,
			axe_attack_speed_pluspercent,
			bow_attack_speed_pluspercent,
			dagger_attack_speed_pluspercent,
			staff_attack_speed_pluspercent,
			claw_attack_speed_pluspercent,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};

		Loaders::StatsValues::Stats varunastra_attack_speed_stats[ ] =
		{
			sword_attack_speed_pluspercent,
			mace_attack_speed_pluspercent,
			axe_attack_speed_pluspercent,
			dagger_attack_speed_pluspercent,
			claw_attack_speed_pluspercent,
		};

		VIRTUAL_STAT( main_hand_attack_speed_pluspercent,
			attack_speed_pluspercent,
			attack_speed_while_dual_wielding_pluspercent, is_dual_wielding,
			melee_attack_speed_pluspercent,
			main_hand_weapon_type,
			main_hand_local_attack_speed_pluspercent,
			two_handed_melee_attack_speed_pluspercent,
			one_handed_melee_attack_speed_pluspercent,
			one_handed_attack_speed_pluspercent,
			attack_speed_pluspercent_per_frenzy_charge, current_frenzy_charges,
			attack_speed_pluspercent_per_izaro_charge, current_izaro_charges,
			on_low_life, on_full_life, attack_speed_pluspercent_when_on_low_life, attack_speed_pluspercent_when_on_full_life,
			monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final,
			sword_attack_speed_pluspercent,
			mace_attack_speed_pluspercent,
			wand_attack_speed_pluspercent,
			axe_attack_speed_pluspercent,
			bow_attack_speed_pluspercent,
			dagger_attack_speed_pluspercent,
			staff_attack_speed_pluspercent,
			claw_attack_speed_pluspercent,
			dual_wield_inherent_attack_speed_pluspercent_final,
			support_multiple_attacks_melee_attack_speed_pluspercent_final, attack_is_melee,
			attack_speed_pluspercent_per_10_dex, dexterity,
			suppressing_fire_debuff_non_melee_attack_speed_pluspercent_final,
			totem_skill_attack_speed_pluspercent, skill_is_totemified,
			echoing_shrine_attack_speed_pluspercent_final,
			attack_and_cast_speed_pluspercent,
			attack_and_cast_speed_pluspercent_while_totem_active, number_of_active_totems,
			active_skill_attack_speed_pluspercent_final,
			support_projectile_attack_speed_pluspercent_final,
			support_attack_totem_attack_speed_pluspercent_final,
			attack_speed_pluspercent_while_ignited, is_ignited,
			attack_speed_pluspercent_while_holding_shield, off_hand_weapon_type,
			attack_speed_pluspercent_per_bloodline_speed_charge, current_bloodline_speed_charges,
			unarmed_melee_attack_speed_pluspercent,
			flicker_strike_more_attack_speed_pluspercent_final,
			attack_speed_pluspercent_with_movement_skills, skill_is_movement_skill,
			attack_speed_pluspercent_while_leeching, is_leeching,
			attack_and_movement_speed_pluspercent_with_her_blessing, have_her_blessing,
			attack_speed_while_fortified_pluspercent, has_fortify,
			virtual_has_onslaught, onslaught_effect_pluspercent,
			attack_speed_pluspercent_during_flask_effect, using_flask,
			virtual_minion_attack_speed_pluspercent, modifiers_to_minion_attack_speed_also_affect_you,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			active_skill_attack_speed_pluspercent_final_per_frenzy_charge,
			melee_ancestor_totem_granted_attack_speed_pluspercent_final,
			is_leeching, attack_and_cast_speed_pluspercent_while_leeching,
			mirror_arrow_and_mirror_arrow_clone_attack_speed_pluspercent, active_skill_index,
			blink_arrow_and_blink_arrow_clone_attack_speed_pluspercent,
			totems_attack_speed_pluspercent_per_active_totem, is_totem,
			attack_and_cast_speed_pluspercent_while_on_consecrated_ground, on_consecrated_ground,
			attack_speed_pluspercent_if_enemy_not_killed_recently, have_killed_in_past_4_seconds,
			attack_speed_pluspercent_per_200_accuracy_rating, main_hand_accuracy_rating,
			attack_and_cast_speed_pluspercent_per_corpse_consumed_recently, number_of_corpses_consumed_recently,
			is_projectile,
			modifiers_to_claw_attack_speed_also_affect_unarmed_attack_speed,
			attack_and_cast_speed_pluspercent_during_flask_effect,
			essence_support_attack_and_cast_speed_pluspercent_final,
			covered_in_spiders_attack_and_cast_speed_pluspercent_final )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			assert( main_hand_weapon_index >= 0 && main_hand_weapon_index <= Items::NumWeaponClasses );

			const auto weapon_type = stats.GetStat( main_hand_weapon_type );
			const bool melee = !!stats.GetStat( attack_is_melee );
			const bool projectile = !!stats.GetStat( is_projectile );
			const bool unarmed = weapon_type == Items::Unarmed;

			const bool is_dual_wielding_value = !!stats.GetStat( is_dual_wielding );
			int main_weapon_speed_increase = 0;

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_attack_speed_stats ), std::end( varunastra_attack_speed_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					main_weapon_speed_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = weapon_attack_speed_stats[size_t( main_hand_weapon_index )] )
					main_weapon_speed_increase += stats.GetStat( stat );
			}

			return Round( ( 100 + stats.GetStat( attack_speed_pluspercent ) +
				( melee ? stats.GetStat( melee_attack_speed_pluspercent ) : 0 ) +
				( is_dual_wielding_value ? stats.GetStat( attack_speed_while_dual_wielding_pluspercent ) : 0 ) +
				(main_weapon_speed_increase)+
				( !main_hand_all_1h_weapons_count && unarmed && stats.GetStat( modifiers_to_claw_attack_speed_also_affect_unarmed_attack_speed ) ? stats.GetStat( weapon_attack_speed_stats[Items::Claw] ) : 0 ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( attack_speed_pluspercent_when_on_low_life ) : 0 ) +
				( stats.GetStat( is_leeching ) ? stats.GetStat( attack_speed_pluspercent_while_leeching ) : 0 ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( attack_speed_while_fortified_pluspercent ) : 0 ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? 0 : stats.GetStat( attack_speed_pluspercent_if_enemy_not_killed_recently ) ) +
				( stats.GetStat( have_her_blessing ) ? stats.GetStat( attack_and_movement_speed_pluspercent_with_her_blessing ) : 0 ) +
				( stats.GetStat( on_full_life ) ? stats.GetStat( attack_speed_pluspercent_when_on_full_life ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_attack_speed_also_affect_you ) ? stats.GetStat( virtual_minion_attack_speed_pluspercent ) : 0 ) +
				( stats.GetStat( is_ignited ) ? stats.GetStat( attack_speed_pluspercent_while_ignited ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? ( 20 * ( 100 + stats.GetStat( onslaught_effect_pluspercent ) ) / 100 ) : 0 ) + //onslaught grants 20% attack/cast/move speeds at base
				( off_hand_weapon_index == Items::Shield ? stats.GetStat( attack_speed_pluspercent_while_holding_shield ) : 0 ) +
				( ( Items::IsTwoHanded[size_t( main_hand_weapon_index )] && Items::IsMelee[size_t( main_hand_weapon_index )] ) ? stats.GetStat( two_handed_melee_attack_speed_pluspercent ) : 0 ) +
				( ( Items::IsOneHanded[size_t( main_hand_weapon_index )] && Items::IsMelee[size_t( main_hand_weapon_index )] ) ? stats.GetStat( one_handed_melee_attack_speed_pluspercent ) : 0 ) +
				( ( stats.GetStat( skill_is_totemified ) && !stats.GetStat( is_totem ) ) ?
					stats.GetStat( totems_attack_speed_pluspercent_per_active_totem ) * stats.GetStat( number_of_active_totems ) : 0 ) +
				( ( Items::IsOneHanded[size_t( main_hand_weapon_index )] ) ? stats.GetStat( one_handed_attack_speed_pluspercent ) : 0 ) +
				stats.GetStat( attack_speed_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				stats.GetStat( attack_speed_pluspercent_per_izaro_charge ) * stats.GetStat( current_izaro_charges ) +
				stats.GetStat( number_of_corpses_consumed_recently ) * stats.GetStat( attack_and_cast_speed_pluspercent_per_corpse_consumed_recently ) +
				( stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_skill_attack_speed_pluspercent ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_on_consecrated_ground ) : 0 ) +
				stats.GetStat( attack_speed_pluspercent_per_10_dex ) * ( stats.GetStat( dexterity ) / 10 ) +
				( stats.GetStat( attack_speed_pluspercent_per_bloodline_speed_charge ) * stats.GetStat( current_bloodline_speed_charges ) ) +
				( ( melee && unarmed ) ? stats.GetStat( unarmed_melee_attack_speed_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_movement_skill ) ? stats.GetStat( attack_speed_pluspercent_with_movement_skills ) : 0 ) +
				( stats.GetStat( using_flask ) ? 
					stats.GetStat( attack_speed_pluspercent_during_flask_effect ) + 
					stats.GetStat( attack_and_cast_speed_pluspercent_during_flask_effect )
					: 0 ) +
				stats.GetStat( attack_and_cast_speed_pluspercent ) +
				stats.GetStat( attack_speed_pluspercent_per_200_accuracy_rating ) * (stats.GetStat( main_hand_accuracy_rating ) / 200 ) +
				( stats.GetStat( is_leeching ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_leeching ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::blink_arrow ) ? stats.GetStat( blink_arrow_and_blink_arrow_clone_attack_speed_pluspercent ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::mirror_arrow ) ? stats.GetStat( mirror_arrow_and_mirror_arrow_clone_attack_speed_pluspercent ) : 0 ) +
				( !!stats.GetStat( number_of_active_totems ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_totem_active ) : 0 ) ) *
				Scale( 100 + stats.GetStat( main_hand_local_attack_speed_pluspercent ) ) *  //local modifier is applied multiplicatively
				Scale( 100 + stats.GetStat( monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final ) ) *
				( melee ? 1 : Scale( 100 + stats.GetStat( suppressing_fire_debuff_non_melee_attack_speed_pluspercent_final ) ) ) *
				( melee ? Scale( 100 + stats.GetStat( support_multiple_attacks_melee_attack_speed_pluspercent_final ) ) : 1 ) *
				( projectile ? Scale( 100 + stats.GetStat( support_projectile_attack_speed_pluspercent_final ) ) : 1 ) *
				( Scale( 100 + stats.GetStat( active_skill_attack_speed_pluspercent_final ) ) ) *
				( is_dual_wielding_value ? Scale( 100 + stats.GetStat( dual_wield_inherent_attack_speed_pluspercent_final ) ) : 1 ) *
				Scale( 100 + stats.GetStat( echoing_shrine_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_attack_totem_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( flicker_strike_more_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( melee_ancestor_totem_granted_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( essence_support_attack_and_cast_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( covered_in_spiders_attack_and_cast_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( active_skill_attack_speed_pluspercent_final_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) )
				- 100 );
		}

		VIRTUAL_STAT( off_hand_attack_speed_pluspercent,
			attack_speed_pluspercent,
			attack_speed_while_dual_wielding_pluspercent, is_dual_wielding,
			melee_attack_speed_pluspercent,
			off_hand_weapon_type,
			off_hand_local_attack_speed_pluspercent,
			two_handed_melee_attack_speed_pluspercent,
			one_handed_melee_attack_speed_pluspercent,
			one_handed_attack_speed_pluspercent,
			attack_speed_pluspercent_per_frenzy_charge, current_frenzy_charges,
			attack_speed_pluspercent_when_on_low_life, on_low_life,
			attack_speed_pluspercent_when_on_full_life, on_full_life,
			monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final,
			attack_speed_pluspercent_per_izaro_charge, current_izaro_charges,
			sword_attack_speed_pluspercent,
			mace_attack_speed_pluspercent,
			wand_attack_speed_pluspercent,
			axe_attack_speed_pluspercent,
			bow_attack_speed_pluspercent,
			dagger_attack_speed_pluspercent,
			staff_attack_speed_pluspercent,
			claw_attack_speed_pluspercent,
			dual_wield_inherent_attack_speed_pluspercent_final,
			support_multiple_attacks_melee_attack_speed_pluspercent_final, attack_is_melee,
			attack_speed_pluspercent_per_10_dex, dexterity,
			support_multiple_attacks_melee_attack_speed_pluspercent_final,
			totem_skill_attack_speed_pluspercent, skill_is_totemified,
			echoing_shrine_attack_speed_pluspercent_final,
			attack_and_cast_speed_pluspercent,
			attack_and_cast_speed_pluspercent_while_totem_active, number_of_active_totems,
			active_skill_attack_speed_pluspercent_final,
			support_projectile_attack_speed_pluspercent_final,
			support_attack_totem_attack_speed_pluspercent_final,
			attack_speed_pluspercent_while_ignited, is_ignited,
			unarmed_melee_attack_speed_pluspercent,
			base_off_hand_attack_speed_pluspercent,
			flicker_strike_more_attack_speed_pluspercent_final,
			attack_and_movement_speed_pluspercent_with_her_blessing, have_her_blessing,
			attack_speed_pluspercent_while_leeching, is_leeching,
			attack_speed_pluspercent_with_movement_skills, skill_is_movement_skill,
			attack_speed_while_fortified_pluspercent, has_fortify,
			virtual_has_onslaught, onslaught_effect_pluspercent,
			attack_speed_pluspercent_during_flask_effect, using_flask,
			virtual_minion_attack_speed_pluspercent, modifiers_to_minion_attack_speed_also_affect_you,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			active_skill_attack_speed_pluspercent_final_per_frenzy_charge,
			melee_ancestor_totem_granted_attack_speed_pluspercent_final,
			is_leeching, attack_and_cast_speed_pluspercent_while_leeching,
			mirror_arrow_and_mirror_arrow_clone_attack_speed_pluspercent, active_skill_index,
			blink_arrow_and_blink_arrow_clone_attack_speed_pluspercent,
			totems_attack_speed_pluspercent_per_active_totem, is_totem,
			attack_and_cast_speed_pluspercent_while_on_consecrated_ground, on_consecrated_ground,
			attack_speed_pluspercent_if_enemy_not_killed_recently, have_killed_in_past_4_seconds,
			attack_speed_pluspercent_per_200_accuracy_rating, off_hand_accuracy_rating,
			attack_and_cast_speed_pluspercent_per_corpse_consumed_recently, number_of_corpses_consumed_recently,
			modifiers_to_claw_attack_speed_also_affect_unarmed_attack_speed, main_hand_weapon_type,
			suppressing_fire_debuff_non_melee_attack_speed_pluspercent_final,
			attack_and_cast_speed_pluspercent_during_flask_effect,
			essence_support_attack_and_cast_speed_pluspercent_final,
			covered_in_spiders_attack_and_cast_speed_pluspercent_final )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const signed int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const signed int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			assert( off_hand_weapon_index >= 0 && off_hand_weapon_index <= Items::NumWeaponClasses );

			const bool melee = !!stats.GetStat( attack_is_melee );
			const bool projectile = !!stats.GetStat( is_projectile );
			const bool unarmed = main_hand_weapon_index == Items::Unarmed;

			const bool is_dual_wielding_value = !!stats.GetStat( is_dual_wielding );

			int off_weapon_speed_increase = 0;

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_attack_speed_stats ), std::end( varunastra_attack_speed_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					off_weapon_speed_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				if ( auto stat = weapon_attack_speed_stats[size_t( off_hand_weapon_index )] )
					off_weapon_speed_increase += stats.GetStat( stat );
			}

			return Round( ( 100 + stats.GetStat( attack_speed_pluspercent ) +
				stats.GetStat( base_off_hand_attack_speed_pluspercent ) +
				( melee ? stats.GetStat( melee_attack_speed_pluspercent ) : 0 ) +
				( is_dual_wielding_value ? stats.GetStat( attack_speed_while_dual_wielding_pluspercent ) : 0 ) +
				( off_weapon_speed_increase ) +
				( !off_hand_all_1h_weapons_count && unarmed && stats.GetStat( modifiers_to_claw_attack_speed_also_affect_unarmed_attack_speed ) ? stats.GetStat( weapon_attack_speed_stats[Items::Claw] ) : 0 ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( attack_speed_pluspercent_when_on_low_life ) : 0 ) +
				( stats.GetStat( is_leeching ) ? stats.GetStat( attack_speed_pluspercent_while_leeching ) : 0 ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? 0 : stats.GetStat( attack_speed_pluspercent_if_enemy_not_killed_recently ) ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( attack_speed_while_fortified_pluspercent ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_attack_speed_also_affect_you ) ? stats.GetStat( virtual_minion_attack_speed_pluspercent ) : 0 ) +
				( stats.GetStat( on_full_life ) ? stats.GetStat( attack_speed_pluspercent_when_on_full_life ) : 0 ) +
				( stats.GetStat( have_her_blessing ) ? stats.GetStat( attack_and_movement_speed_pluspercent_with_her_blessing ) : 0 ) +
				( stats.GetStat( is_ignited ) ? stats.GetStat( attack_speed_pluspercent_while_ignited ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? ( 20 * ( 100 + stats.GetStat( onslaught_effect_pluspercent ) ) / 100 ) : 0 ) + //onslaught grants 20% attack/cast/move speeds at base
				( ( Items::IsTwoHanded[size_t( off_hand_weapon_index )] && Items::IsMelee[size_t( off_hand_weapon_index )] ) ? stats.GetStat( two_handed_melee_attack_speed_pluspercent ) : 0 ) +
				( ( Items::IsOneHanded[size_t( off_hand_weapon_index )] && Items::IsMelee[size_t( off_hand_weapon_index )] ) ? stats.GetStat( one_handed_melee_attack_speed_pluspercent ) : 0 ) +
				( ( stats.GetStat( skill_is_totemified ) && !stats.GetStat( is_totem ) ) ? 
					stats.GetStat( totems_attack_speed_pluspercent_per_active_totem ) * stats.GetStat( number_of_active_totems ) : 0 ) +
				( ( Items::IsOneHanded[size_t( off_hand_weapon_index )] ) ? stats.GetStat( one_handed_attack_speed_pluspercent ) : 0 ) +
				stats.GetStat( attack_speed_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				stats.GetStat( attack_speed_pluspercent_per_izaro_charge ) * stats.GetStat( current_izaro_charges ) +
				stats.GetStat( number_of_corpses_consumed_recently ) * stats.GetStat( attack_and_cast_speed_pluspercent_per_corpse_consumed_recently ) +
				( stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_skill_attack_speed_pluspercent ) : 0 ) +
				stats.GetStat( attack_speed_pluspercent_per_10_dex ) * ( stats.GetStat( dexterity ) / 10 ) +
				( ( melee && unarmed ) ? stats.GetStat( unarmed_melee_attack_speed_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_movement_skill ) ? stats.GetStat( attack_speed_pluspercent_with_movement_skills ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( using_flask ) ? 
					stats.GetStat( attack_speed_pluspercent_during_flask_effect ) + 
					stats.GetStat( attack_and_cast_speed_pluspercent_during_flask_effect )
					: 0 ) +
				stats.GetStat( attack_and_cast_speed_pluspercent ) +
				( stats.GetStat( is_leeching ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_leeching ) : 0 ) +
				stats.GetStat( attack_speed_pluspercent_per_200_accuracy_rating ) * ( stats.GetStat( off_hand_accuracy_rating ) / 200 ) +
				( ( stats.GetStat( active_skill_index ) ==  Loaders::ActiveSkillsValues::blink_arrow ) ? stats.GetStat( blink_arrow_and_blink_arrow_clone_attack_speed_pluspercent ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) ==  Loaders::ActiveSkillsValues::mirror_arrow ) ? stats.GetStat( mirror_arrow_and_mirror_arrow_clone_attack_speed_pluspercent ) : 0 ) +
				( !!stats.GetStat( number_of_active_totems ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_totem_active ) : 0 ) ) *
				Scale( 100 + stats.GetStat( off_hand_local_attack_speed_pluspercent ) ) *  //local modifier is applied multiplicatively
				Scale( 100 + stats.GetStat( monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final ) ) *
				( melee ? 1 : Scale( 100 + stats.GetStat( suppressing_fire_debuff_non_melee_attack_speed_pluspercent_final ) ) ) *
				( melee ? Scale( 100 + stats.GetStat( support_multiple_attacks_melee_attack_speed_pluspercent_final ) ) : 1 ) *
				( projectile ? Scale( 100 + stats.GetStat( support_projectile_attack_speed_pluspercent_final ) ) : 1 ) *
				( is_dual_wielding_value ? Scale( 100 + stats.GetStat( dual_wield_inherent_attack_speed_pluspercent_final ) ) : 1 ) *
				Scale( 100 + stats.GetStat( echoing_shrine_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( active_skill_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( support_attack_totem_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( flicker_strike_more_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( melee_ancestor_totem_granted_attack_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( essence_support_attack_and_cast_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( covered_in_spiders_attack_and_cast_speed_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( active_skill_attack_speed_pluspercent_final_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) )
				- 100 );
		}

		VIRTUAL_STAT( cast_speed_pluspercent,
			base_cast_speed_pluspercent,
			cast_speed_pluspercent_per_power_charge, current_power_charges,
			cast_speed_pluspercent_per_frenzy_charge, current_frenzy_charges,
			cast_speed_pluspercent_per_izaro_charge, current_izaro_charges,
			cast_speed_pluspercent_when_on_low_life, on_low_life,
			cast_speed_pluspercent_when_on_full_life, on_full_life,
			active_skill_cast_speed_pluspercent_final,
			suppressing_fire_debuff_cast_speed_pluspercent_final,
			totem_skill_cast_speed_pluspercent, skill_is_totemified, is_totem,
			echoing_shrine_cast_speed_pluspercent_final,
			attack_and_cast_speed_pluspercent,
			attack_and_cast_speed_pluspercent_while_totem_active, number_of_active_totems,
			support_spell_totem_cast_speed_pluspercent_final,
			support_multicast_cast_speed_pluspercent_final,
			cast_speed_pluspercent_while_ignited, is_ignited,
			cast_speed_while_dual_wielding_pluspercent, is_dual_wielding,
			cast_speed_pluspercent_per_bloodline_speed_charge, current_bloodline_speed_charges,
			cast_speed_pluspercent_while_holding_shield, off_hand_weapon_type,
			cast_speed_pluspercent_while_holding_staff, main_hand_weapon_type,
			skill_is_fire_skill, cast_speed_for_fire_skills_pluspercent,
			skill_is_cold_skill, cast_speed_for_cold_skills_pluspercent,
			skill_is_lightning_skill, cast_speed_for_lightning_skills_pluspercent,
			skill_is_chaos_skill, cast_speed_for_chaos_skills_pluspercent,
			is_leeching, attack_and_cast_speed_pluspercent_while_leeching,
			virtual_has_onslaught, onslaught_effect_pluspercent,
			virtual_minion_cast_speed_pluspercent, modifiers_to_minion_cast_speed_also_affect_you,
			totems_spells_cast_speed_pluspercent_per_active_totem,
			attack_and_cast_speed_pluspercent_while_on_consecrated_ground, on_consecrated_ground,
			attack_and_cast_speed_pluspercent_per_corpse_consumed_recently, number_of_corpses_consumed_recently,
			attack_and_cast_speed_pluspercent_during_flask_effect, using_flask,
			essence_support_attack_and_cast_speed_pluspercent_final,
			covered_in_spiders_attack_and_cast_speed_pluspercent_final )
		{
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool is_using_shield = off_hand_weapon_index == Items::Shield;
			const bool is_using_staff = main_hand_weapon_index == Items::Staff;

			const auto result = Round(
				( ( 100 + stats.GetStat( base_cast_speed_pluspercent ) +
					( stats.GetStat( cast_speed_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) ) +
					( stats.GetStat( cast_speed_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
					( stats.GetStat( cast_speed_pluspercent_per_izaro_charge ) * stats.GetStat( current_izaro_charges ) ) +
					( stats.GetStat( on_low_life ) ? stats.GetStat( cast_speed_pluspercent_when_on_low_life ) : 0 ) +
					( stats.GetStat( on_full_life ) ? stats.GetStat( cast_speed_pluspercent_when_on_full_life ) : 0 ) +
					( stats.GetStat( modifiers_to_minion_cast_speed_also_affect_you ) ? stats.GetStat( virtual_minion_cast_speed_pluspercent ) : 0 ) +
					( stats.GetStat( is_ignited ) ? stats.GetStat( cast_speed_pluspercent_while_ignited ) : 0 ) +
					( stats.GetStat( is_dual_wielding ) ? stats.GetStat( cast_speed_while_dual_wielding_pluspercent ) : 0 ) +
					( ( stats.GetStat( skill_is_totemified ) && !stats.GetStat( is_totem ) ) ? //applies to skills used by totems, but not summoning totems
						stats.GetStat( totem_skill_cast_speed_pluspercent ) +
						stats.GetStat( totems_spells_cast_speed_pluspercent_per_active_totem ) * stats.GetStat( number_of_active_totems ) : 0 ) +
					( stats.GetStat( cast_speed_pluspercent_per_bloodline_speed_charge ) * stats.GetStat( current_bloodline_speed_charges ) ) +
					( is_using_shield ? stats.GetStat( cast_speed_pluspercent_while_holding_shield ) : 0 ) +
					( is_using_staff ? stats.GetStat( cast_speed_pluspercent_while_holding_staff ) : 0 ) +
					( stats.GetStat( virtual_has_onslaught ) ? ( 20 * ( 100 + stats.GetStat( onslaught_effect_pluspercent ) ) / 100 ) : 0 ) + //onslaught grants 20% attack/cast/move speeds at base
					stats.GetStat( attack_and_cast_speed_pluspercent ) +
					stats.GetStat( number_of_corpses_consumed_recently ) * stats.GetStat( attack_and_cast_speed_pluspercent_per_corpse_consumed_recently ) +
					( stats.GetStat( is_leeching ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_leeching ) : 0 ) +
					( !!stats.GetStat( number_of_active_totems ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_totem_active ) : 0 ) +
					( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( attack_and_cast_speed_pluspercent_while_on_consecrated_ground ) : 0 ) +
					( stats.GetStat( using_flask ) ? stats.GetStat( attack_and_cast_speed_pluspercent_during_flask_effect ) : 0 ) +
					( stats.GetStat( skill_is_fire_skill ) ? stats.GetStat( cast_speed_for_fire_skills_pluspercent ) : 0 ) +
					( stats.GetStat( skill_is_cold_skill ) ? stats.GetStat( cast_speed_for_cold_skills_pluspercent ) : 0 ) +
					( stats.GetStat( skill_is_lightning_skill ) ? stats.GetStat( cast_speed_for_lightning_skills_pluspercent ) : 0 ) +
					( stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( cast_speed_for_chaos_skills_pluspercent ) : 0 ) ) *
					Scale( 100 + stats.GetStat( active_skill_cast_speed_pluspercent_final ) ) *
					Scale( 100 + stats.GetStat( suppressing_fire_debuff_cast_speed_pluspercent_final ) ) *
					Scale( 100 + stats.GetStat( support_multicast_cast_speed_pluspercent_final ) ) *
					Scale( 100 + stats.GetStat( echoing_shrine_cast_speed_pluspercent_final ) ) *
					Scale( 100 + stats.GetStat( essence_support_attack_and_cast_speed_pluspercent_final ) ) *
					Scale( 100 + stats.GetStat( covered_in_spiders_attack_and_cast_speed_pluspercent_final ) ) *
					( stats.GetStat( is_totem ) ? 1 : Scale( 100 + stats.GetStat( support_spell_totem_cast_speed_pluspercent_final ) ) )
					- 100 )
				); // multiplicative stat
			
			// Do not allow cast speed to fall below -99%, will induce instance crashes otherwise!
			return std::max( -99, result );
		}

		VIRTUAL_STAT( spectre_maximum_life_pluspercent,
			minion_maximum_life_pluspercent,
			base_spectre_maximum_life_pluspercent )
		{
			return stats.GetStat( minion_maximum_life_pluspercent ) + stats.GetStat( base_spectre_maximum_life_pluspercent );
		}

		VIRTUAL_STAT( zombie_maximum_life_pluspercent,
			minion_maximum_life_pluspercent,
			base_zombie_maximum_life_pluspercent )
		{
			return stats.GetStat( minion_maximum_life_pluspercent ) + stats.GetStat( base_zombie_maximum_life_pluspercent );
		}

		VIRTUAL_STAT( skeleton_maximum_life_pluspercent,
			minion_maximum_life_pluspercent,
			base_skeleton_maximum_life_pluspercent )
		{
			return stats.GetStat( minion_maximum_life_pluspercent ) + stats.GetStat( base_skeleton_maximum_life_pluspercent );
		}

		VIRTUAL_STAT( fire_elemental_maximum_life_pluspercent,
			minion_maximum_life_pluspercent,
			base_fire_elemental_maximum_life_pluspercent )
		{
			return stats.GetStat( minion_maximum_life_pluspercent ) + stats.GetStat( base_fire_elemental_maximum_life_pluspercent );
		}

		VIRTUAL_STAT( raven_maximum_life_pluspercent,
			minion_maximum_life_pluspercent,
			base_raven_maximum_life_pluspercent )
		{
			return stats.GetStat( minion_maximum_life_pluspercent ) + stats.GetStat( base_raven_maximum_life_pluspercent );
		}

		VIRTUAL_STAT( number_of_additional_arrows,
			base_number_of_additional_arrows,
			virtual_number_of_additional_projectiles )
		{
			return stats.GetStat( base_number_of_additional_arrows ) + stats.GetStat( virtual_number_of_additional_projectiles );
		}

		VIRTUAL_STAT( projectile_speed_pluspercent,
			base_projectile_speed_pluspercent,
			projectile_speed_pluspercent_per_frenzy_charge, current_frenzy_charges,
			support_slower_projectiles_projectile_speed_pluspercent_final )
		{
			return Round( ( 100 + stats.GetStat( base_projectile_speed_pluspercent ) +
				stats.GetStat( projectile_speed_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) *
				Scale( 100 + stats.GetStat( support_slower_projectiles_projectile_speed_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( arrow_speed_pluspercent,
			base_arrow_speed_pluspercent,
			base_projectile_speed_pluspercent,
			projectile_speed_pluspercent_per_frenzy_charge, current_frenzy_charges,
			support_slower_projectiles_projectile_speed_pluspercent_final )
		{
			return Round( ( 100 + stats.GetStat( base_arrow_speed_pluspercent ) +
				stats.GetStat( base_projectile_speed_pluspercent ) +
				stats.GetStat( projectile_speed_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) *
				Scale( 100 + stats.GetStat( support_slower_projectiles_projectile_speed_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( arrow_pierce_percent,
			base_arrow_pierce_percent,
			virtual_pierce_percent )
		{
			return std::min( 100, stats.GetStat( base_arrow_pierce_percent ) + stats.GetStat( virtual_pierce_percent ) );
		}

		VIRTUAL_STAT( main_hand_stun_threshold_reduction_pluspercent,
			base_stun_threshold_reduction_pluspercent,
			while_using_mace_stun_threshold_reduction_pluspercent, main_hand_weapon_type,
			bow_stun_threshold_reduction_pluspercent,
			main_hand_local_stun_threshold_reduction_pluspercent,
			stun_threshold_reduction_pluspercent_while_using_flask, using_flask,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const bool is_using_mace = main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::OneHandMace || main_hand_weapon_index == Items::TwoHandMace || main_hand_weapon_index == Items::Sceptre;
			const bool is_using_bow = main_hand_weapon_index == Items::Bow;
			return stats.GetStat( base_stun_threshold_reduction_pluspercent ) +
				( is_using_mace ? stats.GetStat( while_using_mace_stun_threshold_reduction_pluspercent ) : 0 ) +
				( is_using_bow ? stats.GetStat( bow_stun_threshold_reduction_pluspercent ) : 0 ) +
				stats.GetStat( main_hand_local_stun_threshold_reduction_pluspercent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( stun_threshold_reduction_pluspercent_while_using_flask ) : 0 );
		}

		VIRTUAL_STAT( off_hand_stun_threshold_reduction_pluspercent,
			base_stun_threshold_reduction_pluspercent,
			while_using_mace_stun_threshold_reduction_pluspercent, off_hand_weapon_type,
			bow_stun_threshold_reduction_pluspercent, main_hand_weapon_type,
			off_hand_local_stun_threshold_reduction_pluspercent,
			stun_threshold_reduction_pluspercent_while_using_flask, using_flask,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const bool is_using_mace = off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::OneHandMace || off_hand_weapon_index == Items::TwoHandMace || off_hand_weapon_index == Items::Sceptre;
			const bool is_using_bow = main_hand_weapon_index == Items::Bow;
			return stats.GetStat( base_stun_threshold_reduction_pluspercent ) +
				( is_using_mace ? stats.GetStat( while_using_mace_stun_threshold_reduction_pluspercent ) : 0 ) +
				( is_using_bow ? stats.GetStat( bow_stun_threshold_reduction_pluspercent ) : 0 ) +
				stats.GetStat( off_hand_local_stun_threshold_reduction_pluspercent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( stun_threshold_reduction_pluspercent_while_using_flask ) : 0 );
		}
		//TODO: convert % stats to permyriad. Or change them all?
		//TODO: Scale old leech
		VIRTUAL_STAT( main_hand_life_leech_from_physical_attack_damage_permyriad,
			life_leech_from_physical_attack_damage_permyriad, old_do_not_use_life_leech_from_physical_damage_percent,
			life_leech_from_physical_damage_with_bow_permyriad, main_hand_weapon_type,
			life_leech_from_physical_damage_with_claw_permyriad, old_do_not_use_life_leech_from_physical_damage_with_claw_percent,
			main_hand_local_life_leech_from_physical_damage_permyriad, old_do_not_use_main_hand_local_life_leech_from_physical_damage_percent,
			life_and_mana_leech_from_physical_damage_permyriad,
			main_hand_local_life_and_mana_leech_from_physical_damage_permyriad,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const bool is_using_claw = main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Claw;
			const bool is_using_bow = main_hand_weapon_index == Items::Bow;
			return stats.GetStat( life_leech_from_physical_attack_damage_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_life_leech_from_physical_damage_percent ) ) +
				( is_using_bow ? stats.GetStat( life_leech_from_physical_damage_with_bow_permyriad ) : 0 ) +
				( is_using_claw ? stats.GetStat( life_leech_from_physical_damage_with_claw_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_life_leech_from_physical_damage_with_claw_percent ) ) : 0 ) +
				stats.GetStat( main_hand_local_life_leech_from_physical_damage_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_main_hand_local_life_leech_from_physical_damage_percent ) ) +
				stats.GetStat( life_and_mana_leech_from_physical_damage_permyriad ) +
				stats.GetStat( main_hand_local_life_and_mana_leech_from_physical_damage_permyriad );
		}

		VIRTUAL_STAT( off_hand_life_leech_from_physical_attack_damage_permyriad,
			life_leech_from_physical_attack_damage_permyriad, old_do_not_use_life_leech_from_physical_damage_percent,
			off_hand_weapon_type, life_leech_from_physical_damage_with_claw_permyriad, old_do_not_use_life_leech_from_physical_damage_with_claw_percent,
			off_hand_local_life_leech_from_physical_damage_permyriad, old_do_not_use_off_hand_local_life_leech_from_physical_damage_percent,
			life_and_mana_leech_from_physical_damage_permyriad,
			main_hand_local_life_and_mana_leech_from_physical_damage_permyriad,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool is_using_claw = off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Claw;
			return stats.GetStat( life_leech_from_physical_attack_damage_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_life_leech_from_physical_damage_percent ) ) +
				( is_using_claw ? stats.GetStat( life_leech_from_physical_damage_with_claw_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_life_leech_from_physical_damage_with_claw_percent ) ) : 0 ) +
				( stats.GetStat( off_hand_local_life_leech_from_physical_damage_permyriad ) ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_off_hand_local_life_leech_from_physical_damage_percent ) ) +
				( stats.GetStat( life_and_mana_leech_from_physical_damage_permyriad ) ) +
				( stats.GetStat( main_hand_local_life_and_mana_leech_from_physical_damage_permyriad ) );
		}

		VIRTUAL_STAT( main_hand_mana_leech_from_physical_attack_damage_permyriad,
			mana_leech_from_physical_attack_damage_permyriad, old_do_not_use_mana_leech_from_physical_damage_percent,
			main_hand_local_mana_leech_from_physical_damage_permyriad, old_do_not_use_main_hand_local_mana_leech_from_physical_damage_percent,
			mana_leech_from_physical_damage_with_bow_permyriad, main_hand_weapon_type,
			mana_leech_from_physical_damage_with_claw_permyriad, old_do_not_use_mana_leech_from_physical_damage_with_claw_percent,
			current_power_charges, mana_leech_from_physical_damage_permyriad_per_power_charge, old_do_not_use_mana_leech_from_physical_damage_percent_per_power_charge,
			life_and_mana_leech_from_physical_damage_permyriad,
			main_hand_local_life_and_mana_leech_from_physical_damage_permyriad,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const bool is_using_claw = main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Claw;
			const bool is_using_bow = main_hand_weapon_index == Items::Bow;
			return stats.GetStat( mana_leech_from_physical_attack_damage_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_physical_damage_percent ) ) +
				stats.GetStat( main_hand_local_mana_leech_from_physical_damage_permyriad ) + +ScaleOldPercentLeech( stats.GetStat( old_do_not_use_main_hand_local_mana_leech_from_physical_damage_percent ) ) +
				( is_using_claw ? stats.GetStat( mana_leech_from_physical_damage_with_claw_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_physical_damage_with_claw_percent ) ) : 0 ) +
				( is_using_bow ? stats.GetStat( mana_leech_from_physical_damage_with_bow_permyriad ) : 0 ) +
				( stats.GetStat( mana_leech_from_physical_damage_permyriad_per_power_charge ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_physical_damage_percent_per_power_charge ) ) ) * stats.GetStat( current_power_charges ) +
				stats.GetStat( life_and_mana_leech_from_physical_damage_permyriad ) +
				stats.GetStat( main_hand_local_life_and_mana_leech_from_physical_damage_permyriad );
		}

		VIRTUAL_STAT( off_hand_mana_leech_from_physical_attack_damage_permyriad,
			mana_leech_from_physical_attack_damage_permyriad, old_do_not_use_mana_leech_from_physical_damage_percent,
			off_hand_local_mana_leech_from_physical_damage_permyriad, old_do_not_use_off_hand_local_mana_leech_from_physical_damage_percent,
			off_hand_weapon_type, mana_leech_from_physical_damage_with_claw_permyriad, old_do_not_use_mana_leech_from_physical_damage_with_claw_percent,
			current_power_charges, mana_leech_from_physical_damage_permyriad_per_power_charge, old_do_not_use_mana_leech_from_physical_damage_percent_per_power_charge,
			life_and_mana_leech_from_physical_damage_permyriad,
			off_hand_local_life_and_mana_leech_from_physical_damage_permyriad,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool is_using_claw = off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Claw;
			return stats.GetStat( mana_leech_from_physical_attack_damage_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_physical_damage_percent ) ) +
				stats.GetStat( off_hand_local_mana_leech_from_physical_damage_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_off_hand_local_mana_leech_from_physical_damage_percent ) ) +
				( is_using_claw ? stats.GetStat( mana_leech_from_physical_damage_with_claw_permyriad ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_physical_damage_with_claw_percent ) ) : 0 ) +
				( stats.GetStat( mana_leech_from_physical_damage_permyriad_per_power_charge ) + ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_physical_damage_percent_per_power_charge ) ) ) * stats.GetStat( current_power_charges ) +
				( stats.GetStat( life_and_mana_leech_from_physical_damage_permyriad ) ) +
				( stats.GetStat( off_hand_local_life_and_mana_leech_from_physical_damage_permyriad ) );
		}

		/*
		 * These ones have virtual versions now as a means of containing the old leech scaling here, so other code doesn't need to interact with it directly.
		 */
		VIRTUAL_STAT( life_leech_from_spell_damage_permyriad,
			base_life_leech_from_spell_damage_permyriad,
			old_do_not_use_life_leech_from_spell_damage_percent )
		{
			return stats.GetStat( base_life_leech_from_spell_damage_permyriad ) +
				ScaleOldPercentLeech( stats.GetStat( old_do_not_use_life_leech_from_spell_damage_percent ) );
		}

		VIRTUAL_STAT( mana_leech_from_spell_damage_permyriad,
			base_mana_leech_from_spell_damage_permyriad,
			old_do_not_use_mana_leech_from_spell_damage_percent )
		{
			return stats.GetStat( base_mana_leech_from_spell_damage_permyriad ) +
				ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_from_spell_damage_percent ) );
		}

		VIRTUAL_STAT( life_leech_permyriad_vs_frozen_enemies,
			base_life_leech_permyriad_vs_frozen_enemies,
			old_do_not_use_life_leech_percent_vs_frozen_enemies )
		{
			return stats.GetStat( base_life_leech_permyriad_vs_frozen_enemies ) +
				ScaleOldPercentLeech( stats.GetStat( old_do_not_use_life_leech_percent_vs_frozen_enemies ) );
		}

		VIRTUAL_STAT( life_leech_from_attack_damage_permyriad_vs_chilled_enemies,
			base_life_leech_from_attack_damage_permyriad_vs_chilled_enemies,
			old_do_not_use_life_leech_from_attack_damage_permyriad_vs_chilled_enemies )
		{
			return stats.GetStat( base_life_leech_from_attack_damage_permyriad_vs_chilled_enemies ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_life_leech_from_attack_damage_permyriad_vs_chilled_enemies ) );
		}

		VIRTUAL_STAT( life_leech_permyriad_vs_shocked_enemies,
			base_life_leech_permyriad_vs_shocked_enemies,
			old_do_not_use_life_leech_permyriad_vs_shocked_enemies )
		{
			return stats.GetStat( base_life_leech_permyriad_vs_shocked_enemies ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_life_leech_permyriad_vs_shocked_enemies ) );
		}

		VIRTUAL_STAT( mana_leech_permyriad_vs_shocked_enemies,
			base_mana_leech_permyriad_vs_shocked_enemies,
			old_do_not_use_mana_leech_percent_vs_shocked_enemies, )
		{
			return stats.GetStat( base_mana_leech_permyriad_vs_shocked_enemies ) +
				ScaleOldPercentLeech( stats.GetStat( old_do_not_use_mana_leech_percent_vs_shocked_enemies ) );
		}

		VIRTUAL_STAT( maximum_fire_damage_resistance_percent,
			base_maximum_fire_damage_resistance_percent,
			additional_maximum_all_resistances_percent,
			additional_maximum_all_elemental_resistances_percent )
		{
			return stats.GetStat( base_maximum_fire_damage_resistance_percent ) + stats.GetStat( additional_maximum_all_resistances_percent ) + stats.GetStat( additional_maximum_all_elemental_resistances_percent );
		}

		VIRTUAL_STAT( maximum_cold_damage_resistance_percent,
			base_maximum_cold_damage_resistance_percent,
			additional_maximum_all_resistances_percent,
			additional_maximum_all_elemental_resistances_percent )
		{
			return stats.GetStat( base_maximum_cold_damage_resistance_percent ) + stats.GetStat( additional_maximum_all_resistances_percent ) + stats.GetStat( additional_maximum_all_elemental_resistances_percent );
		}

		VIRTUAL_STAT( maximum_lightning_damage_resistance_percent,
			base_maximum_lightning_damage_resistance_percent,
			additional_maximum_all_resistances_percent,
			additional_maximum_all_elemental_resistances_percent )
		{
			return stats.GetStat( base_maximum_lightning_damage_resistance_percent ) + stats.GetStat( additional_maximum_all_resistances_percent ) + stats.GetStat( additional_maximum_all_elemental_resistances_percent );
		}

		VIRTUAL_STAT( maximum_chaos_damage_resistance_percent,
			base_maximum_chaos_damage_resistance_percent,
			additional_maximum_all_resistances_percent )
		{
			return stats.GetStat( base_maximum_chaos_damage_resistance_percent ) + stats.GetStat( additional_maximum_all_resistances_percent );
		}

		VIRTUAL_STAT( total_fire_damage_resistance_percent,
			base_fire_damage_resistance_percent,
			fire_and_cold_damage_resistance_percent,
			fire_and_lightning_damage_resistance_percent,
			resist_all_elements_percent,
			elemental_resistance_percent_when_on_low_life, on_low_life,
			fire_damage_resistance_percent_when_on_low_life,
			modifiers_to_minion_resistances_also_affect_you, minion_elemental_resistance_percent )
		{
			return stats.GetStat( base_fire_damage_resistance_percent ) +
				stats.GetStat( resist_all_elements_percent ) +
				stats.GetStat( fire_and_cold_damage_resistance_percent ) +
				stats.GetStat( fire_and_lightning_damage_resistance_percent ) +
				( stats.GetStat( on_low_life ) ? ( stats.GetStat( elemental_resistance_percent_when_on_low_life ) + stats.GetStat( fire_damage_resistance_percent_when_on_low_life ) ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_resistances_also_affect_you ) ? stats.GetStat( minion_elemental_resistance_percent ) : 0 );
		}

		VIRTUAL_STAT( fire_damage_resistance_percent,
			maximum_fire_damage_resistance_percent,
			total_fire_damage_resistance_percent,
			zero_elemental_resistance,
			fire_damage_immunity )
		{
			if ( stats.GetStat( fire_damage_immunity ) )
				return 100;

			if ( stats.GetStat( zero_elemental_resistance ) )
				return 0;

			return std::min( stats.GetStat( maximum_fire_damage_resistance_percent ),
				stats.GetStat( total_fire_damage_resistance_percent ) );
		}

		VIRTUAL_STAT( total_cold_damage_resistance_percent,
			base_cold_damage_resistance_percent,
			resist_all_elements_percent,
			fire_and_cold_damage_resistance_percent,
			cold_and_lightning_damage_resistance_percent,
			elemental_resistance_percent_when_on_low_life, on_low_life,
			modifiers_to_minion_resistances_also_affect_you, minion_elemental_resistance_percent,
			minion_cold_damage_resistance_percent )
		{

			return stats.GetStat( base_cold_damage_resistance_percent ) +
				stats.GetStat( resist_all_elements_percent ) +
				stats.GetStat( fire_and_cold_damage_resistance_percent ) +
				stats.GetStat( cold_and_lightning_damage_resistance_percent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( elemental_resistance_percent_when_on_low_life ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_resistances_also_affect_you ) ? stats.GetStat( minion_elemental_resistance_percent ) + stats.GetStat( minion_cold_damage_resistance_percent ) : 0 );
		}

		VIRTUAL_STAT( cold_damage_resistance_percent,
			maximum_cold_damage_resistance_percent,
			total_cold_damage_resistance_percent,
			cannot_resist_cold_damage,
			zero_elemental_resistance,
			cold_damage_immunity )
		{
			if ( stats.GetStat( cold_damage_immunity ) )
				return 100;

			if ( stats.GetStat( zero_elemental_resistance ) )
				return 0;

			const auto max = stats.GetStat( cannot_resist_cold_damage ) ? 0 : stats.GetStat( maximum_cold_damage_resistance_percent );

			return std::min( max,
				stats.GetStat( total_cold_damage_resistance_percent ) );
		}

		VIRTUAL_STAT( total_lightning_damage_resistance_percent,
			base_lightning_damage_resistance_percent,
			resist_all_elements_percent,
			fire_and_lightning_damage_resistance_percent,
			cold_and_lightning_damage_resistance_percent,
			elemental_resistance_percent_when_on_low_life, on_low_life,
			modifiers_to_minion_resistances_also_affect_you, minion_elemental_resistance_percent )
		{
			return stats.GetStat( base_lightning_damage_resistance_percent ) +
				stats.GetStat( resist_all_elements_percent ) +
				stats.GetStat( fire_and_lightning_damage_resistance_percent ) +
				stats.GetStat( cold_and_lightning_damage_resistance_percent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( elemental_resistance_percent_when_on_low_life ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_resistances_also_affect_you ) ? stats.GetStat( minion_elemental_resistance_percent ) : 0 );
		}

		VIRTUAL_STAT( lightning_damage_resistance_percent,
			maximum_lightning_damage_resistance_percent,
			total_lightning_damage_resistance_percent,
			zero_elemental_resistance,
			lightning_damage_immunity )
		{
			if ( stats.GetStat( lightning_damage_immunity ) )
				return 100;

			if ( stats.GetStat( zero_elemental_resistance ) )
				return 0;

			return std::min( stats.GetStat( maximum_lightning_damage_resistance_percent ),
				stats.GetStat( total_lightning_damage_resistance_percent ) );
		}

		VIRTUAL_STAT( total_chaos_damage_resistance_percent,
			base_chaos_damage_resistance_percent,
			chaos_damage_resistance_percent_when_on_low_life, on_low_life,
			modifiers_to_minion_resistances_also_affect_you, minion_chaos_resistance_percent,
			chaos_resistance_plus_while_using_flask, using_flask )
		{
			return stats.GetStat( base_chaos_damage_resistance_percent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( chaos_damage_resistance_percent_when_on_low_life ) : 0 ) +
				( stats.GetStat( modifiers_to_minion_resistances_also_affect_you ) ? stats.GetStat( minion_chaos_resistance_percent ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( chaos_resistance_plus_while_using_flask ) : 0 );
		}

		VIRTUAL_STAT( chaos_damage_resistance_percent,
			maximum_chaos_damage_resistance_percent,
			total_chaos_damage_resistance_percent,
			chaos_damage_immunity )
		{
			if ( stats.GetStat( chaos_damage_immunity ) )
				return 100; //100% resistance if immune

			return std::min( stats.GetStat( maximum_chaos_damage_resistance_percent ),
				stats.GetStat( total_chaos_damage_resistance_percent ) );
		}

		VIRTUAL_STAT( resist_all_elements_percent,
			base_resist_all_elements_percent,
			resist_all_elements_percent_per_endurance_charge, current_endurance_charges,
			resist_all_elements_percent_per_izaro_charge, current_izaro_charges,
			resist_all_elements_pluspercent_while_holding_shield, off_hand_weapon_type,
			resist_all_elements_percent_per_10_levels, level )
		{
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const int shield_resist = ( off_hand_weapon_index == Items::Shield ? stats.GetStat( resist_all_elements_pluspercent_while_holding_shield ) : 0 );
			return stats.GetStat( base_resist_all_elements_percent ) +
				( stats.GetStat( resist_all_elements_percent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) +
				( stats.GetStat( resist_all_elements_percent_per_izaro_charge ) * stats.GetStat( current_izaro_charges ) ) +
				( stats.GetStat( resist_all_elements_percent_per_10_levels ) * stats.GetStat( level ) / 10 ) +
				shield_resist;
		}

		VIRTUAL_STAT( fire_damage_immunity,
			base_fire_immunity,
			elemental_damage_immunity )
		{
			return stats.GetStat( base_fire_immunity ) || stats.GetStat( elemental_damage_immunity );
		}

		VIRTUAL_STAT( cold_damage_immunity,
			base_cold_immunity,
			elemental_damage_immunity )
		{
			return stats.GetStat( base_cold_immunity ) || stats.GetStat( elemental_damage_immunity );
		}

		VIRTUAL_STAT( lightning_damage_immunity,
			base_lightning_immunity,
			elemental_damage_immunity )
		{
			return stats.GetStat( base_lightning_immunity ) || stats.GetStat( elemental_damage_immunity );
		}

		VIRTUAL_STAT( chaos_damage_immunity,
			chaos_immunity,
			keystone_chaos_inoculation )
		{
			return stats.GetStat( chaos_immunity ) || stats.GetStat( keystone_chaos_inoculation );
		}

		//stats to simplify conversion of evastion rating to physical damage reduction rating (armour)
		VIRTUAL_STAT( total_base_evasion_rating,
			base_evasion_rating,
			evasion_rating_per_level, level,
			evasion_rating_plus_when_on_low_life, on_low_life,
			evasion_rating_plus_when_on_full_life, on_full_life,
			base_physical_damage_reduction_and_evasion_rating )
		{
			return stats.GetStat( base_evasion_rating ) +
				stats.GetStat( base_physical_damage_reduction_and_evasion_rating ) +
				stats.GetStat( evasion_rating_per_level ) * stats.GetStat( level ) / 100 + //evasion_rating_per_level is stored as x100 for more granularity
				( stats.GetStat( on_low_life ) ? stats.GetStat( evasion_rating_plus_when_on_low_life ) : 0 ) +
				( stats.GetStat( on_full_life ) ? stats.GetStat( evasion_rating_plus_when_on_full_life ) : 0 );
		}

		VIRTUAL_STAT( combined_evasion_rating_pluspercent,
			evasion_rating_pluspercent,
			evasion_rating_pluspercent_per_frenzy_charge, current_frenzy_charges,
			dexterity, keystone_iron_reflexes,
			evasion_and_physical_damage_reduction_rating_pluspercent,
			evasion_rating_pluspercent_when_on_low_life, on_low_life,
			global_defences_pluspercent,
			evasion_rating_pluspercent_while_onslaught_is_active, virtual_has_onslaught,
			armour_and_evasion_on_low_life_pluspercent,
			evasion_pluspercent_if_hit_recently, have_been_hit_in_past_4_seconds,
			armour_and_evasion_rating_pluspercent_if_killed_a_taunted_enemy_recently, have_killed_a_taunted_enemy_recently,
			armour_and_evasion_pluspercent_while_fortified, has_fortify,
			evasion_pluspercent_per_10_intelligence, intelligence )
		{
			const int evasion_increase_from_dexterity = Round( stats.GetStat( dexterity ) / 5.0f ); // 5 point dex = 1% increased evasion

			return ( stats.GetStat( keystone_iron_reflexes ) ? 0 : evasion_increase_from_dexterity ) +
				stats.GetStat( evasion_rating_pluspercent ) +
				stats.GetStat( evasion_rating_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				stats.GetStat( evasion_and_physical_damage_reduction_rating_pluspercent ) +
				( stats.GetStat( on_low_life ) ?
					stats.GetStat( evasion_rating_pluspercent_when_on_low_life ) +
					stats.GetStat( armour_and_evasion_on_low_life_pluspercent ) : 0 ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( armour_and_evasion_pluspercent_while_fortified ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? stats.GetStat( evasion_rating_pluspercent_while_onslaught_is_active ) : 0 ) +
				stats.GetStat( global_defences_pluspercent ) +
				( stats.GetStat( have_been_hit_in_past_4_seconds ) ? stats.GetStat( evasion_pluspercent_if_hit_recently ) : 0 ) +
				( stats.GetStat( have_killed_a_taunted_enemy_recently ) ? stats.GetStat( armour_and_evasion_rating_pluspercent_if_killed_a_taunted_enemy_recently ) : 0 ) +
				( stats.GetStat( evasion_pluspercent_per_10_intelligence ) * int( stats.GetStat( intelligence ) / 10 ) );
		}

		VIRTUAL_STAT( combined_evasion_rating_from_shield_pluspercent,
			combined_evasion_rating_pluspercent,
			shield_evasion_rating_pluspercent,
			shield_armour_pluspercent )
		{
			return stats.GetStat( combined_evasion_rating_pluspercent ) +
				stats.GetStat( shield_evasion_rating_pluspercent ) +
				stats.GetStat( shield_armour_pluspercent );
		}

		VIRTUAL_STAT( evasion_rating,
			total_base_evasion_rating,
			combined_evasion_rating_pluspercent,
			base_shield_evasion_rating,
			combined_evasion_rating_from_shield_pluspercent,
			keystone_iron_reflexes,
			grace_aura_evasion_rating_pluspercent_final,
			evasion_rating_pluspercent_final_from_poachers_mark,
			minions_get_shield_stats_instead_of_you,
			evasion_rating_while_es_full_pluspercent_final, on_full_energy_shield )
		{
			if ( stats.GetStat( keystone_iron_reflexes ) )
				return 0;

			return std::max( Round( (
				stats.GetStat( total_base_evasion_rating ) * Scale( 100 + stats.GetStat( combined_evasion_rating_pluspercent ) ) +
				( stats.GetStat( minions_get_shield_stats_instead_of_you ) ? 0 : stats.GetStat( base_shield_evasion_rating ) * Scale( 100 + stats.GetStat( combined_evasion_rating_from_shield_pluspercent ) ) )
				) *
				Scale( 100 + stats.GetStat( grace_aura_evasion_rating_pluspercent_final ) ) *
				Scale( 100 + ( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( evasion_rating_while_es_full_pluspercent_final ) : 0 ) ) *
				Scale( 100 + stats.GetStat( evasion_rating_pluspercent_final_from_poachers_mark ) ) ), 0 );
		}

		VIRTUAL_STAT( physical_damage_reduction_rating,
			base_physical_damage_reduction_rating,
			base_body_armour_physical_damage_reduction_rating, damage_reduction_rating_from_body_armour_doubled,
			physical_damage_reduction_rating_pluspercent,
			base_shield_physical_damage_reduction_rating,
			shield_physical_damage_reduction_rating_pluspercent,
			shield_armour_pluspercent,
			no_physical_damage_reduction_rating,
			keystone_iron_reflexes,
			evasion_and_physical_damage_reduction_rating_pluspercent,
			total_base_evasion_rating,
			combined_evasion_rating_pluspercent,
			grace_aura_evasion_rating_pluspercent_final,
			base_shield_evasion_rating,
			combined_evasion_rating_from_shield_pluspercent,
			determination_aura_armour_pluspercent_final,
			keystone_acrobatics_physical_damage_reduction_rating_pluspercent_final,
			physical_damage_reduction_rating_per_level, level,
			physical_damage_reduction_rating_plus1percent_per_X_strength, strength,
			physical_damage_reduction_rating_while_frozen, is_frozen,
			physical_damage_reduction_rating_pluspercent_while_not_ignited_frozen_shocked, is_ignited, is_frozen, is_shocked,
			global_defences_pluspercent,
			physical_damage_reduction_rating_pluspercent_while_fortify_is_active, has_fortify,
			armour_and_evasion_on_low_life_pluspercent, on_low_life,
			physical_damage_reduction_rating_pluspercent_while_chilled_or_frozen, is_chilled,
			minions_get_shield_stats_instead_of_you,
			number_of_stackable_unique_jewels, X_armour_per_stackable_unique_jewel,
			armour_and_evasion_rating_pluspercent_if_killed_a_taunted_enemy_recently, have_killed_a_taunted_enemy_recently,
			armour_and_evasion_pluspercent_while_fortified,
			base_physical_damage_reduction_and_evasion_rating,
			maximum_life_percent_to_add_as_maximum_armour, total_base_maximum_life, keystone_chaos_inoculation )
		{
			if ( stats.GetStat( no_physical_damage_reduction_rating ) )
				return 0;

			const bool ignited = !!stats.GetStat( is_ignited );
			const bool frozen = !!stats.GetStat( is_frozen );
			const bool chilled = !!stats.GetStat( is_chilled );
			const bool shocked = !!stats.GetStat( is_shocked );

			//armour
			const auto base_physical_reduction = 
				stats.GetStat( base_physical_damage_reduction_rating ) +
				stats.GetStat( base_physical_damage_reduction_and_evasion_rating ) +
				( stats.GetStat( base_body_armour_physical_damage_reduction_rating ) * ( stats.GetStat( damage_reduction_rating_from_body_armour_doubled ) ? 2 : 1 ) ) +
				stats.GetStat( physical_damage_reduction_rating_per_level ) * stats.GetStat( level ) +
				( frozen ? stats.GetStat( physical_damage_reduction_rating_while_frozen ) : 0 ) + 
				stats.GetStat( number_of_stackable_unique_jewels ) * stats.GetStat( X_armour_per_stackable_unique_jewel );

			const auto base_shield_physical_reduction = stats.GetStat( base_shield_physical_damage_reduction_rating );

			//armour increases that specifically _only_ apply to armour. These stack additively with the evasion increases for evasion that's converted to armour
			const auto physical_only_reduction_increase =
				stats.GetStat( physical_damage_reduction_rating_pluspercent ) +
				Divide( stats.GetStat( strength ), stats.GetStat( physical_damage_reduction_rating_plus1percent_per_X_strength ) ) +
				( ( ignited || frozen || shocked ) ? 0 : stats.GetStat( physical_damage_reduction_rating_pluspercent_while_not_ignited_frozen_shocked ) ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( physical_damage_reduction_rating_pluspercent_while_fortify_is_active ) : 0 ) +
				( ( frozen || chilled ) ? stats.GetStat( physical_damage_reduction_rating_pluspercent_while_chilled_or_frozen ) : 0 );

			//includes armour increases that can/do also apply to evasion - seperate so they aren't applied again to converted evasion, which already gets them from the combined_evasion_rating_pluspercent stat
			const auto all_physical_reduction_increase =
				physical_only_reduction_increase +
				stats.GetStat( global_defences_pluspercent ) +
				stats.GetStat( evasion_and_physical_damage_reduction_rating_pluspercent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( armour_and_evasion_on_low_life_pluspercent ) : 0 ) +
				( stats.GetStat( has_fortify ) ? stats.GetStat( armour_and_evasion_pluspercent_while_fortified ) : 0 ) +
				( stats.GetStat( have_killed_a_taunted_enemy_recently ) ? stats.GetStat( armour_and_evasion_rating_pluspercent_if_killed_a_taunted_enemy_recently ) : 0 );

			const auto physical_only_reduction_increase_on_shield = physical_only_reduction_increase + stats.GetStat( shield_physical_damage_reduction_rating_pluspercent );
			const auto all_physical_reduction_increase_on_shield = all_physical_reduction_increase + stats.GetStat( shield_physical_damage_reduction_rating_pluspercent ) + stats.GetStat( shield_armour_pluspercent );

			float total_from_evasion = 0.0f;
			if ( stats.GetStat( keystone_iron_reflexes ) )
			{
				total_from_evasion = stats.GetStat( total_base_evasion_rating ) * Scale( 100 + stats.GetStat( combined_evasion_rating_pluspercent ) + physical_only_reduction_increase );
				if ( !stats.GetStat( minions_get_shield_stats_instead_of_you ) )
					total_from_evasion += stats.GetStat( base_shield_evasion_rating ) * Scale( 100 + stats.GetStat( combined_evasion_rating_from_shield_pluspercent ) + physical_only_reduction_increase_on_shield );
			}

			float armour_from_life = 0.0f;
			if ( const auto percent_add_life = stats.GetStat( maximum_life_percent_to_add_as_maximum_armour ) )
			{
				if ( stats.GetStat( keystone_chaos_inoculation ) )
				{
					armour_from_life = 1.0f;
				}
				else
				{
					//convert to life with applicable increases summed.
					armour_from_life = stats.GetStat( total_base_maximum_life ) *
						Scale( percent_add_life ) *
						Scale( 100 + stats.GetStat( combined_life_pluspercent ) + all_physical_reduction_increase ) *
						Scale( 100 + stats.GetStat( combined_life_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( determination_aura_armour_pluspercent_final ) ) *
						Scale( 100 + stats.GetStat( keystone_acrobatics_physical_damage_reduction_rating_pluspercent_final ) );
				}
			}

			return Round(
				(
					base_physical_reduction * Scale( 100 + all_physical_reduction_increase ) +
					( stats.GetStat( minions_get_shield_stats_instead_of_you ) ? 0 : base_shield_physical_reduction * Scale( 100 + all_physical_reduction_increase_on_shield ) ) +
					
					total_from_evasion * 
					Scale( 100 + stats.GetStat( grace_aura_evasion_rating_pluspercent_final ) ) *
					Scale( 100 + ( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( evasion_rating_while_es_full_pluspercent_final ) : 0 ) ) *
					Scale( 100 + stats.GetStat( evasion_rating_pluspercent_final_from_poachers_mark ) )
				) * Scale( 100 + stats.GetStat( determination_aura_armour_pluspercent_final ) ) * Scale( 100 + stats.GetStat( keystone_acrobatics_physical_damage_reduction_rating_pluspercent_final ) )
				+ armour_from_life //separate because for CI to work (makes it 1 after all modifiers), all multipliecaitve armour modifiers were already included in this value
				);
		}

		VIRTUAL_STAT( physical_damage_reduction_rating_against_projectiles,
			base_physical_damage_reduction_rating,
			base_physical_damage_reduction_and_evasion_rating,
			evasion_and_physical_damage_reduction_rating_pluspercent,
			grace_aura_evasion_rating_pluspercent_final,
			base_body_armour_physical_damage_reduction_rating, damage_reduction_rating_from_body_armour_doubled,
			physical_damage_reduction_rating_pluspercent_against_projectiles,
			physical_damage_reduction_rating_pluspercent,
			base_shield_physical_damage_reduction_rating,
			shield_physical_damage_reduction_rating_pluspercent,
			shield_armour_pluspercent,
			no_physical_damage_reduction_rating,
			keystone_iron_reflexes,
			total_base_evasion_rating,
			combined_evasion_rating_pluspercent,
			base_shield_evasion_rating,
			combined_evasion_rating_from_shield_pluspercent,
			determination_aura_armour_pluspercent_final,
			keystone_acrobatics_physical_damage_reduction_rating_pluspercent_final,
			global_defences_pluspercent,
			minions_get_shield_stats_instead_of_you )
		{
			if ( stats.GetStat( no_physical_damage_reduction_rating ) )
				return 0;

			//armour
			const auto base_physical_reduction = stats.GetStat( base_physical_damage_reduction_rating ) +
				stats.GetStat( base_physical_damage_reduction_and_evasion_rating ) +
				( stats.GetStat( base_body_armour_physical_damage_reduction_rating ) * stats.GetStat( damage_reduction_rating_from_body_armour_doubled ) ? 2 : 1 );

			const auto base_shield_physical_reduction = stats.GetStat( base_shield_physical_damage_reduction_rating );
			//armour increases
			const auto physical_only_reduction_increase = stats.GetStat( physical_damage_reduction_rating_pluspercent ) + stats.GetStat( physical_damage_reduction_rating_pluspercent_against_projectiles );;
			const auto all_physical_reduction_increase = physical_only_reduction_increase + stats.GetStat( evasion_and_physical_damage_reduction_rating_pluspercent ) + stats.GetStat( global_defences_pluspercent );
			const auto physical_only_reduction_increase_on_shield = physical_only_reduction_increase + stats.GetStat( shield_physical_damage_reduction_rating_pluspercent );
			const auto all_physical_reduction_increase_on_shield = all_physical_reduction_increase + stats.GetStat( shield_physical_damage_reduction_rating_pluspercent ) + stats.GetStat( shield_armour_pluspercent );

			float total_from_evasion = 0.0f;
			if ( stats.GetStat( keystone_iron_reflexes ) )
			{
				total_from_evasion = stats.GetStat( total_base_evasion_rating ) * Scale( 100 + stats.GetStat( combined_evasion_rating_pluspercent ) + physical_only_reduction_increase );
				if ( stats.GetStat( minions_get_shield_stats_instead_of_you ) )
					total_from_evasion += stats.GetStat( base_shield_evasion_rating ) * Scale( 100 + stats.GetStat( combined_evasion_rating_from_shield_pluspercent ) + physical_only_reduction_increase_on_shield );
			}

			float armour_from_life = 0.0f;
			if ( const auto percent_add_life = stats.GetStat( maximum_life_percent_to_add_as_maximum_armour ) )
			{
				if ( stats.GetStat( keystone_chaos_inoculation ) )
				{
					armour_from_life = 1.0f;
				}
				else
				{
					//convert to life with applicable increases summed.
					armour_from_life = stats.GetStat( total_base_maximum_life ) *
						Scale( percent_add_life ) *
						Scale( 100 + stats.GetStat( combined_life_pluspercent ) ) *
						Scale( 100 + stats.GetStat( combined_life_pluspercent_final ) );
				}
			}

			return Round(
				(
					base_physical_reduction * Scale( 100 + all_physical_reduction_increase ) +
					( stats.GetStat( minions_get_shield_stats_instead_of_you ) ? 0 : base_shield_physical_reduction * Scale( 100 + all_physical_reduction_increase_on_shield ) ) +

					total_from_evasion * 
					Scale( 100 + stats.GetStat( grace_aura_evasion_rating_pluspercent_final ) ) *
					Scale( 100 + ( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( evasion_rating_while_es_full_pluspercent_final ) : 0 ) ) *
					Scale( 100 + stats.GetStat( evasion_rating_pluspercent_final_from_poachers_mark ) ) +

					armour_from_life

					) * Scale( 100 + stats.GetStat( determination_aura_armour_pluspercent_final ) ) * Scale( 100 + stats.GetStat( keystone_acrobatics_physical_damage_reduction_rating_pluspercent_final ) ) );
		}

		//this stat only used for attack damage - spell/secondary damage just uses base_stun_duration_pluspercent
		VIRTUAL_STAT( stun_duration_pluspercent,
			base_stun_duration_pluspercent,
			two_handed_melee_stun_duration_pluspercent,
			staff_stun_duration_pluspercent,
			bow_stun_duration_pluspercent,
			main_hand_weapon_type )
		{
			//We do not need to have main hand / off hand specific stats here because both of the weapon specific stats are for two handed weapons.
			const unsigned weapon_type = stats.GetStat( main_hand_weapon_type );

			unsigned weapon_specific_increases = 0;
			if ( Items::IsTwoHanded[size_t( weapon_type )] && Items::IsMelee[size_t( weapon_type )] )
				weapon_specific_increases += stats.GetStat( two_handed_melee_stun_duration_pluspercent );
			if ( weapon_type == Items::Staff )
				weapon_specific_increases += stats.GetStat( staff_stun_duration_pluspercent );
			else if ( weapon_type == Items::Bow )
				weapon_specific_increases += stats.GetStat( bow_stun_duration_pluspercent );

			return stats.GetStat( base_stun_duration_pluspercent ) + weapon_specific_increases;
		}

		VIRTUAL_STAT( skill_effect_duration,
			base_skill_effect_duration,
			virtual_additional_skill_effect_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final )
		{
			return
				Round( ( stats.GetStat( base_skill_effect_duration ) +
					stats.GetStat( virtual_additional_skill_effect_duration ) ) *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) )  *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}


		VIRTUAL_STAT( projectile_ground_effect_duration,
			base_projectile_ground_effect_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final )
		{
			return
				Round( ( stats.GetStat( base_projectile_ground_effect_duration ) ) *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) )  *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( secondary_skill_effect_duration,
			base_secondary_skill_effect_duration,
			virtual_additional_skill_effect_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final )
		{
			return
				Round( ( stats.GetStat( base_secondary_skill_effect_duration ) +
					stats.GetStat( virtual_additional_skill_effect_duration ) ) *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) ) *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( virtual_skill_effect_duration_pluspercent,
			skill_effect_duration_pluspercent,
			skill_effect_duration_pluspercent_per_10_strength, strength,
			vaal_skill_effect_duration_pluspercent, skill_is_vaal_skill,
			chaos_skill_effect_duration_pluspercent, skill_is_chaos_skill,
			skill_effect_duration_pluspercent_per_frenzy_charge, current_frenzy_charges,
			skill_effect_duration_per_100_int_pluspercent, intelligence,
			skill_effect_duration_pluspercent_if_killed_maimed_enemy_recently, have_killed_a_maimed_enemy_recently )

		{
			return stats.GetStat( skill_effect_duration_pluspercent ) +
				( stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_effect_duration_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_skill_effect_duration_pluspercent ) : 0 ) +
				( stats.GetStat( have_killed_a_maimed_enemy_recently ) ? stats.GetStat( skill_effect_duration_pluspercent_if_killed_maimed_enemy_recently ) : 0 ) +
				( stats.GetStat( skill_effect_duration_pluspercent_per_10_strength ) * ( stats.GetStat( strength ) / 10 ) +
					stats.GetStat( skill_effect_duration_per_100_int_pluspercent ) * ( stats.GetStat( intelligence ) / 100 ) ) +
				( stats.GetStat( skill_effect_duration_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) );
		}

		VIRTUAL_STAT( virtual_skill_effect_duration_pluspercent_final,
			support_reduced_duration_skill_effect_duration_pluspercent_final )
		{
			return Round( 100 *
				Scale( 100 + stats.GetStat( support_reduced_duration_skill_effect_duration_pluspercent_final ) ) - 100 );
		}

		VIRTUAL_STAT( virtual_additional_skill_effect_duration,
			skill_effect_duration_per_100_int, intelligence )
		{
			return stats.GetStat( skill_effect_duration_per_100_int ) * ( stats.GetStat( intelligence ) / 100 );
		}

		VIRTUAL_STAT( buff_effect_duration,
			base_skill_effect_duration,
			virtual_additional_skill_effect_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final,
			buff_duration_pluspercent,
			buff_effect_duration_pluspercent_per_endurance_charge, current_endurance_charges,
			base_buff_duration_ms_plus_per_endurance_charge,
			warcry_duration_pluspercent, is_warcry )
		{
			const auto discrete = ( ( stats.GetStat( base_skill_effect_duration ) ) +
				( stats.GetStat( base_buff_duration_ms_plus_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) +
				( stats.GetStat( virtual_additional_skill_effect_duration ) ) );

			const auto pluspercent = Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) +
				( stats.GetStat( buff_duration_pluspercent ) ) +
				( stats.GetStat( is_warcry ) ? stats.GetStat( warcry_duration_pluspercent ) : 0 ) +
				( stats.GetStat( buff_effect_duration_pluspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) );

			const auto pluspercentfinal = Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) );

			return Round( discrete * pluspercent * pluspercentfinal );
		}

		VIRTUAL_STAT( secondary_buff_effect_duration,
			base_secondary_skill_effect_duration,
			virtual_additional_skill_effect_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final,
			buff_duration_pluspercent,
			buff_effect_duration_pluspercent_per_endurance_charge, current_endurance_charges,
			base_buff_duration_ms_plus_per_endurance_charge )
		{
			return
				Round
				(
					( stats.GetStat( base_secondary_skill_effect_duration ) + stats.GetStat( base_buff_duration_ms_plus_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) +
						stats.GetStat( virtual_additional_skill_effect_duration ) * Scale(
							100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) + stats.GetStat( buff_duration_pluspercent ) +
							stats.GetStat( buff_effect_duration_pluspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) *
						Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) )
					);
		}

		VIRTUAL_STAT( curse_effect_duration,
			base_skill_effect_duration,
			virtual_additional_skill_effect_duration,
			virtual_skill_effect_duration_pluspercent_final,
			curse_duration_pluspercent )
		{
			return Round( ( stats.GetStat( base_skill_effect_duration ) + stats.GetStat( virtual_additional_skill_effect_duration ) ) *
				Scale( 100 + stats.GetStat( curse_duration_pluspercent ) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( curse_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent,
			buff_duration_pluspercent,
			buff_effect_duration_pluspercent_per_endurance_charge, current_endurance_charges,
			base_curse_duration_pluspercent )
		{
			return stats.GetStat( virtual_skill_effect_duration_pluspercent ) + stats.GetStat( buff_duration_pluspercent ) +
				( stats.GetStat( buff_effect_duration_pluspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) +
				stats.GetStat( base_curse_duration_pluspercent );
		}

		VIRTUAL_STAT( skill_area_of_effect_pluspercent,
			base_skill_area_of_effect_pluspercent,
			skill_area_of_effect_pluspercent_per_power_charge, current_power_charges,
			area_of_effect_pluspercent_per_20_int, intelligence,
			area_of_effect_pluspercent_while_dead, is_dead,
			support_concentrated_effect_skill_area_of_effect_pluspercent_final,
			skill_area_of_effect_when_unarmed_pluspercent, main_hand_weapon_type,
			totem_skill_area_of_effect_pluspercent, skill_is_totemified,
			skill_area_of_effect_pluspercent_per_active_mine, number_of_active_mines,
			trap_skill_area_of_effect_pluspercent, skill_is_trapped,
			base_aura_area_of_effect_pluspercent, skill_is_aura_skill,
			curse_area_of_effect_pluspercent, skill_is_curse,
			skill_area_of_effect_pluspercent_final,
			skill_area_of_effect_pluspercent_if_enemy_killed_recently, have_killed_in_past_4_seconds,
			attack_area_of_effect_pluspercent, skill_is_attack )
		{
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			bool unarmed = main_hand_weapon_index == Items::Unarmed;

			return Round( ( 100 +
				stats.GetStat( base_skill_area_of_effect_pluspercent ) +
				stats.GetStat( skill_area_of_effect_pluspercent_per_power_charge ) * stats.GetStat( current_power_charges ) +
				stats.GetStat( skill_area_of_effect_pluspercent_per_active_mine ) * stats.GetStat( number_of_active_mines ) +
				stats.GetStat( area_of_effect_pluspercent_per_20_int ) * ( stats.GetStat( intelligence ) / 20 ) +
				( unarmed ? stats.GetStat( skill_area_of_effect_when_unarmed_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_skill_area_of_effect_pluspercent ) : 0 ) +
				( stats.GetStat( is_dead ) ? stats.GetStat( area_of_effect_pluspercent_while_dead ) : 0 ) +
				( stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_skill_area_of_effect_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_attack ) ? stats.GetStat( attack_area_of_effect_pluspercent ) : 0 ) +
				( stats.GetStat( skill_is_curse ) ? stats.GetStat( curse_area_of_effect_pluspercent ) : 0 ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? stats.GetStat( skill_area_of_effect_pluspercent_if_enemy_killed_recently ) : 0 ) +
				( stats.GetStat( skill_is_aura_skill ) ? stats.GetStat( base_aura_area_of_effect_pluspercent ) : 0 ) ) *
				Scale( 100 + stats.GetStat( support_concentrated_effect_skill_area_of_effect_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( skill_area_of_effect_pluspercent_final ) ) - 100 );
		}

		VIRTUAL_STAT( avoid_interruption_percent,
			avoid_interruption_while_casting_percent, casting_spell )
		{
			return stats.GetStat( casting_spell ) ? stats.GetStat( avoid_interruption_while_casting_percent ) : 0;
		}

		VIRTUAL_STAT( add_power_charge_when_interrupted,
			add_power_charge_when_interrupted_while_casting, casting_spell )
		{
			return stats.GetStat( casting_spell ) ? stats.GetStat( add_power_charge_when_interrupted_while_casting ) : 0;
		}

		VIRTUAL_STAT( shocks_enemies_that_hit_actor,
			shocks_enemies_that_hit_actor_while_actor_is_casting, casting_spell )
		{
			return stats.GetStat( casting_spell ) ? stats.GetStat( shocks_enemies_that_hit_actor_while_actor_is_casting ) : 0;
		}

		VIRTUAL_STAT( additional_physical_damage_reduction_percent,
			physical_damage_reduction_percent_per_endurance_charge, current_endurance_charges,
			physical_damage_reduction_percent_per_izaro_charge, current_izaro_charges,
			physical_damage_reduction_and_minion_physical_damage_reduction_percent_per_raised_zombie, number_of_active_zombies,
			additional_physical_damage_reduction_percent_when_on_low_life, on_low_life,
			damage_reduction_rating_percent_with_active_totem, number_of_active_totems,
			base_additional_physical_damage_reduction_percent,
			physical_damage_reduction_and_minion_physical_damage_reduction_percent )
		{
			return stats.GetStat( current_endurance_charges ) * stats.GetStat( physical_damage_reduction_percent_per_endurance_charge ) +
				stats.GetStat( current_izaro_charges ) * stats.GetStat( physical_damage_reduction_percent_per_izaro_charge ) +
				( stats.GetStat( number_of_active_totems ) > 0 ? stats.GetStat( damage_reduction_rating_percent_with_active_totem ) : 0 ) +
				stats.GetStat( number_of_active_zombies ) * stats.GetStat( physical_damage_reduction_and_minion_physical_damage_reduction_percent_per_raised_zombie ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( additional_physical_damage_reduction_percent_when_on_low_life ) : 0 ) +
				stats.GetStat( base_additional_physical_damage_reduction_percent ) +
				stats.GetStat( physical_damage_reduction_and_minion_physical_damage_reduction_percent );
		}

		VIRTUAL_STAT( main_hand_poison_on_critical_strike,
			poison_on_critical_strike_with_dagger, main_hand_weapon_type,
			poison_on_critical_strike_with_bow,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( poison_on_critical_strike_with_dagger ) && ( main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Dagger ) ||
				stats.GetStat( poison_on_critical_strike_with_bow ) && main_hand_weapon_index == Items::Bow;
		}

		VIRTUAL_STAT( off_hand_poison_on_critical_strike,
			poison_on_critical_strike_with_dagger, off_hand_weapon_type,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( poison_on_critical_strike_with_dagger ) && ( off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Dagger );
		}

		VIRTUAL_STAT( main_hand_chance_to_poison_on_critical_strike_percent,
			chance_to_poison_on_critical_strike_with_dagger_percent, main_hand_weapon_type,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return ( main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Dagger ? stats.GetStat( chance_to_poison_on_critical_strike_with_dagger_percent ) : 0 );
		}

		VIRTUAL_STAT( off_hand_chance_to_poison_on_critical_strike_percent,
			chance_to_poison_on_critical_strike_with_dagger_percent, off_hand_weapon_type,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return ( off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Dagger ? stats.GetStat( chance_to_poison_on_critical_strike_with_dagger_percent ) : 0 );
		}


		Loaders::StatsValues::Stats weapon_critical_strike_chance_stats[ ] =
		{
			sword_critical_strike_chance_pluspercent,
			sword_critical_strike_chance_pluspercent,
			sword_critical_strike_chance_pluspercent,
			mace_critical_strike_chance_pluspercent,
			mace_critical_strike_chance_pluspercent,
			mace_critical_strike_chance_pluspercent,
			wand_critical_strike_chance_pluspercent,
			axe_critical_strike_chance_pluspercent,
			axe_critical_strike_chance_pluspercent,
			bow_critical_strike_chance_pluspercent,
			dagger_critical_strike_chance_pluspercent,
			staff_critical_strike_chance_pluspercent,
			claw_critical_strike_chance_pluspercent,
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 ),
			Loaders::StatsValues::Stats( 0 )
		};


		Loaders::StatsValues::Stats varunastra_critical_strike_chance_stats[ ] =
		{
			sword_critical_strike_chance_pluspercent,
			mace_critical_strike_chance_pluspercent,
			axe_critical_strike_chance_pluspercent,
			dagger_critical_strike_chance_pluspercent,
			claw_critical_strike_chance_pluspercent,
		};

		VIRTUAL_STAT( main_hand_critical_strike_chance,
			main_hand_local_critical_strike_chance, additional_base_critical_strike_chance,
			base_critical_strike_chance_while_unarmed_percent,
			additional_critical_strike_chance_per_power_charge_permyriad,
			main_hand_local_critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent_per_power_charge, current_power_charges,
			critical_strike_chance_pluspercent_per_izaro_charge, current_izaro_charges,
			critical_strike_chance_pluspercent_per_level, level,
			main_hand_weapon_type,
			dagger_critical_strike_chance_pluspercent,
			bow_critical_strike_chance_pluspercent,
			claw_critical_strike_chance_pluspercent,
			sword_critical_strike_chance_pluspercent,
			mace_critical_strike_chance_pluspercent,
			staff_critical_strike_chance_pluspercent,
			wand_critical_strike_chance_pluspercent,
			one_handed_melee_critical_strike_chance_pluspercent,
			two_handed_melee_critical_strike_chance_pluspercent,
			critical_strike_chance_while_dual_wielding_pluspercent, is_dual_wielding,
			global_critical_strike_chance_while_dual_wielding_pluspercent,
			virtual_global_cannot_crit,
			support_multiple_projectiles_critical_strike_chance_pluspercent_final, is_projectile,
			arrow_pierce_chance_applies_to_projectile_crit_chance, arrow_pierce_percent,
			always_crit, attack_always_crit,
			global_critical_strike_chance_pluspercent_while_holding_staff,
			melee_critical_strike_chance_pluspercent, attack_is_melee,
			melee_critical_strike_chance_pluspercent_when_on_full_life,
			on_full_life,
			maximum_critical_strike_chance,
			unique_critical_strike_chance_pluspercent_final,
			critical_strike_chance_pluspercent_per_8_strength, strength,
			axe_critical_strike_chance_pluspercent,
			mine_critical_strike_chance_pluspercent, skill_is_mined,
			trap_critical_strike_chance_pluspercent, skill_is_trapped,
			critical_strike_chance_while_wielding_shield_pluspercent, off_hand_weapon_type,
			fire_critical_strike_chance_pluspercent, skill_is_fire_skill,
			lightning_critical_strike_chance_pluspercent, skill_is_lightning_skill,
			cold_critical_strike_chance_pluspercent, skill_is_cold_skill,
			elemental_critical_strike_chance_pluspercent,
			chaos_critical_strike_chance_pluspercent, skill_is_chaos_skill,
			vaal_skill_critical_strike_chance_pluspercent, skill_is_vaal_skill,
			totem_critical_strike_chance_pluspercent, skill_is_totemified,
			enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			assassinate_passive_critical_strike_chance_pluspercent_final,
			ambush_passive_critical_strike_chance_pluspercent_final,
			modifiers_to_claw_critical_strike_chance_also_affect_unarmed_critical_strike_chance,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			global_critical_strike_chance_pluspercent_while_holding_bow,
			main_hand_weapon_always_crit )
		{
			if ( stats.GetStat( virtual_global_cannot_crit ) )
				return 0;

			if ( stats.GetStat( always_crit ) || stats.GetStat( attack_always_crit ) || stats.GetStat( main_hand_weapon_always_crit ) )
				return 10000;

			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const auto base_chance =
				stats.GetStat( main_hand_local_critical_strike_chance ) * ( 100 + stats.GetStat( main_hand_local_critical_strike_chance_pluspercent ) ) / 100 + //should match how this is done in weapon.cpp
				stats.GetStat( additional_base_critical_strike_chance ) +
				( main_hand_weapon_index == Items::Unarmed ? stats.GetStat( base_critical_strike_chance_while_unarmed_percent ) : 0 ) +
				stats.GetStat( additional_critical_strike_chance_per_power_charge_permyriad ) * stats.GetStat( current_power_charges );

			if ( base_chance <= 0 )
				return 0;

			const auto max_crit_override = stats.GetStat( maximum_critical_strike_chance );
			const bool has_shield = off_hand_weapon_index == Items::Shield;
			const bool is_elemental = stats.GetStat( skill_is_fire_skill ) || stats.GetStat( skill_is_lightning_skill ) || stats.GetStat( skill_is_cold_skill );

			unsigned crit_chance_increase = 100 + stats.GetStat( critical_strike_chance_pluspercent ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_level ) * stats.GetStat( level ) +
				stats.GetStat( current_power_charges ) * stats.GetStat( critical_strike_chance_pluspercent_per_power_charge ) +
				stats.GetStat( current_izaro_charges ) * stats.GetStat( critical_strike_chance_pluspercent_per_izaro_charge ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_8_strength ) * stats.GetStat( strength ) / 8 +
				( stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_chance_pluspercent ) : 0 );

			if ( main_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_critical_strike_chance_stats ), std::end( varunastra_critical_strike_chance_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					crit_chance_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				const auto weapon_critical_strike_chance_increase_stat = weapon_critical_strike_chance_stats[size_t( main_hand_weapon_index )];
				crit_chance_increase += stats.GetStat( weapon_critical_strike_chance_increase_stat );
			}

			if ( main_hand_weapon_index == Items::Bow )
				crit_chance_increase += stats.GetStat( global_critical_strike_chance_pluspercent_while_holding_bow );
			else if ( !main_hand_all_1h_weapons_count && main_hand_weapon_index == Items::Unarmed && stats.GetStat( modifiers_to_claw_critical_strike_chance_also_affect_unarmed_critical_strike_chance ) )
				crit_chance_increase += stats.GetStat( claw_critical_strike_chance_pluspercent );
			else if ( main_hand_weapon_index == Items::Staff )
				crit_chance_increase += stats.GetStat( global_critical_strike_chance_pluspercent_while_holding_staff );

			// Trapped, totem and mined skills
			crit_chance_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_chance_pluspercent ) : 0;

			// Wielding shield
			crit_chance_increase += has_shield ? stats.GetStat( critical_strike_chance_while_wielding_shield_pluspercent ) : 0;

			// Elemental increases
			crit_chance_increase += stats.GetStat( skill_is_fire_skill ) ? stats.GetStat( fire_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_lightning_skill ) ? stats.GetStat( lightning_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_cold_skill ) ? stats.GetStat( cold_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += is_elemental ? stats.GetStat( elemental_critical_strike_chance_pluspercent ) : 0;

			// Enchantment negative-positive buff.
			if( !stats.GetStat( have_crit_in_past_4_seconds ) )
				crit_chance_increase += stats.GetStat( enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds );

			// Deadeye
			crit_chance_increase += ( stats.GetStat( is_projectile ) && stats.GetStat( arrow_pierce_chance_applies_to_projectile_crit_chance ) ) ?
				stats.GetStat( arrow_pierce_percent ) : 0;

			//melee attacks
			if ( stats.GetStat( attack_is_melee ) )
			{
				crit_chance_increase += stats.GetStat( melee_critical_strike_chance_pluspercent );

				if ( stats.GetStat( on_full_life ) )
					crit_chance_increase += stats.GetStat( melee_critical_strike_chance_pluspercent_when_on_full_life );
			}

			//attacks with melee weapons
			if ( Items::IsMelee[size_t( main_hand_weapon_index )] && Items::IsWeapon[size_t( main_hand_weapon_index )] )
			{
				if ( Items::IsOneHanded[size_t( main_hand_weapon_index )] )
					crit_chance_increase += stats.GetStat( one_handed_melee_critical_strike_chance_pluspercent );
				else if ( Items::IsTwoHanded[size_t( main_hand_weapon_index )] )
					crit_chance_increase += stats.GetStat( two_handed_melee_critical_strike_chance_pluspercent );
			}

			if( stats.GetStat( is_dual_wielding ) )
			{
				crit_chance_increase += stats.GetStat( critical_strike_chance_while_dual_wielding_pluspercent );
				crit_chance_increase += stats.GetStat( global_critical_strike_chance_while_dual_wielding_pluspercent );
			}

			const float final_increase = ( ( !Items::IsMelee[size_t( main_hand_weapon_index )] || stats.GetStat( is_projectile ) ) ?
				Scale( 100 + stats.GetStat( support_multiple_projectiles_critical_strike_chance_pluspercent_final ) ) : 1.0f ) *
				Scale( 100 + stats.GetStat( unique_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( ambush_passive_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( assassinate_passive_critical_strike_chance_pluspercent_final ) );

			return Clamp( Round( base_chance * Scale( crit_chance_increase ) * final_increase ), 500, max_crit_override ? max_crit_override : 9500 );
		}

		VIRTUAL_STAT( off_hand_critical_strike_chance,
			off_hand_local_critical_strike_chance,
			additional_base_critical_strike_chance,
			additional_critical_strike_chance_per_power_charge_permyriad,
			off_hand_local_critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent_per_level, level,
			critical_strike_chance_pluspercent_per_power_charge, current_power_charges,
			off_hand_weapon_type,
			dagger_critical_strike_chance_pluspercent,
			bow_critical_strike_chance_pluspercent,
			claw_critical_strike_chance_pluspercent,
			sword_critical_strike_chance_pluspercent,
			mace_critical_strike_chance_pluspercent,
			wand_critical_strike_chance_pluspercent,
			one_handed_melee_critical_strike_chance_pluspercent,
			two_handed_melee_critical_strike_chance_pluspercent,
			critical_strike_chance_while_dual_wielding_pluspercent,
			global_critical_strike_chance_while_dual_wielding_pluspercent,
			is_dual_wielding, virtual_global_cannot_crit,
			support_multiple_projectiles_critical_strike_chance_pluspercent_final, is_projectile,
			arrow_pierce_chance_applies_to_projectile_crit_chance, arrow_pierce_percent,
			always_crit, attack_always_crit,
			melee_critical_strike_chance_pluspercent, attack_is_melee,
			melee_critical_strike_chance_pluspercent_when_on_full_life, on_full_life,
			maximum_critical_strike_chance,
			unique_critical_strike_chance_pluspercent_final,
			critical_strike_chance_pluspercent_per_8_strength, strength,
			axe_critical_strike_chance_pluspercent,
			mine_critical_strike_chance_pluspercent, skill_is_mined,
			trap_critical_strike_chance_pluspercent, skill_is_trapped,
			critical_strike_chance_while_wielding_shield_pluspercent,
			fire_critical_strike_chance_pluspercent, skill_is_fire_skill,
			lightning_critical_strike_chance_pluspercent, skill_is_lightning_skill,
			cold_critical_strike_chance_pluspercent, skill_is_cold_skill,
			elemental_critical_strike_chance_pluspercent,
			chaos_critical_strike_chance_pluspercent, skill_is_chaos_skill,
			vaal_skill_critical_strike_chance_pluspercent, skill_is_vaal_skill,
			totem_critical_strike_chance_pluspercent, skill_is_totemified,
			enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			assassinate_passive_critical_strike_chance_pluspercent_final,
			ambush_passive_critical_strike_chance_pluspercent_final,
			modifiers_to_claw_critical_strike_chance_also_affect_unarmed_critical_strike_chance,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			off_hand_weapon_always_crit )
		{
			if ( stats.GetStat( virtual_global_cannot_crit ) )
				return 0;

			if ( stats.GetStat( always_crit ) || stats.GetStat( attack_always_crit ) || stats.GetStat( off_hand_weapon_always_crit ) )
				return 10000;

			const auto base_chance =
				stats.GetStat( off_hand_local_critical_strike_chance ) * ( 100 + stats.GetStat( off_hand_local_critical_strike_chance_pluspercent ) ) / 100 +
				stats.GetStat( additional_base_critical_strike_chance ) +
				stats.GetStat( additional_critical_strike_chance_per_power_charge_permyriad ) * stats.GetStat( current_power_charges );

			if ( base_chance <= 0 )
				return 0;

			const auto max_crit_override = stats.GetStat( maximum_critical_strike_chance );
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const unsigned off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );
			const bool is_elemental = stats.GetStat( skill_is_fire_skill ) || stats.GetStat( skill_is_lightning_skill ) || stats.GetStat( skill_is_cold_skill );

			unsigned crit_chance_increase = 100 + stats.GetStat( critical_strike_chance_pluspercent ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_level ) * stats.GetStat( level ) +
				stats.GetStat( current_power_charges ) * stats.GetStat( critical_strike_chance_pluspercent_per_power_charge ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_8_strength ) * stats.GetStat( strength ) / 8 +
				( stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_chance_pluspercent ) : 0 );

			if ( off_hand_all_1h_weapons_count )
			{
				std::for_each( std::begin( varunastra_critical_strike_chance_stats ), std::end( varunastra_critical_strike_chance_stats ), [&]( const Loaders::StatsValues::Stats stat )
				{
					crit_chance_increase += stats.GetStat( stat );
				} );
			}
			else
			{
				const auto weapon_critical_strike_chance_increase_stat = weapon_critical_strike_chance_stats[size_t( off_hand_weapon_index )];
				crit_chance_increase += stats.GetStat( weapon_critical_strike_chance_increase_stat );
			}

			if ( !off_hand_all_1h_weapons_count && off_hand_weapon_index == Items::Unarmed && stats.GetStat( modifiers_to_claw_critical_strike_chance_also_affect_unarmed_critical_strike_chance ) )
				crit_chance_increase += stats.GetStat( claw_critical_strike_chance_pluspercent );

			// Trapped, totem and mined skills
			crit_chance_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_chance_pluspercent ) : 0;

			// Wielding shields
			crit_chance_increase += ( off_hand_weapon_index == Items::Shield ) ? stats.GetStat( critical_strike_chance_while_wielding_shield_pluspercent ) : 0;

			// Elemental increases
			crit_chance_increase += stats.GetStat( skill_is_fire_skill ) ? stats.GetStat( fire_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_lightning_skill ) ? stats.GetStat( lightning_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_cold_skill ) ? stats.GetStat( cold_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += is_elemental ? stats.GetStat( elemental_critical_strike_chance_pluspercent ) : 0;

			// Enchantment negative-positive buff.
			if( !stats.GetStat( have_crit_in_past_4_seconds ) )
				crit_chance_increase += stats.GetStat( enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds );

			// Deadeye
			crit_chance_increase += ( stats.GetStat( is_projectile ) && stats.GetStat( arrow_pierce_chance_applies_to_projectile_crit_chance ) ) ?
				stats.GetStat( arrow_pierce_percent ) : 0;

			//melee attacks
			if ( stats.GetStat( attack_is_melee ) )
			{
				crit_chance_increase += stats.GetStat( melee_critical_strike_chance_pluspercent );

				if ( stats.GetStat( on_full_life ) )
					crit_chance_increase += stats.GetStat( melee_critical_strike_chance_pluspercent_when_on_full_life );
			}

			//attacks with melee weapons
			if ( Items::IsMelee[size_t( off_hand_weapon_index )] && Items::IsWeapon[size_t( off_hand_weapon_index )] )
			{
				if ( Items::IsOneHanded[size_t( off_hand_weapon_index )] )
					crit_chance_increase += stats.GetStat( one_handed_melee_critical_strike_chance_pluspercent );
				else if ( Items::IsTwoHanded[size_t( off_hand_weapon_index )] )
					crit_chance_increase += stats.GetStat( two_handed_melee_critical_strike_chance_pluspercent );
			}

			if( stats.GetStat( is_dual_wielding ) )
			{
				crit_chance_increase += stats.GetStat( critical_strike_chance_while_dual_wielding_pluspercent );
				crit_chance_increase += stats.GetStat( global_critical_strike_chance_while_dual_wielding_pluspercent );
			}

			const float final_increase = ( ( !Items::IsMelee[size_t( off_hand_weapon_index )] || stats.GetStat( is_projectile ) ) ?
				Scale( 100 + stats.GetStat( support_multiple_projectiles_critical_strike_chance_pluspercent_final ) ) : 1.0f ) *
				Scale( 100 + stats.GetStat( unique_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( ambush_passive_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( assassinate_passive_critical_strike_chance_pluspercent_final ) );

			return Clamp( Round( base_chance * Scale( crit_chance_increase ) * final_increase ), 500, max_crit_override ? max_crit_override : 9500 );
		}

		VIRTUAL_STAT( spell_critical_strike_chance,
			base_spell_critical_strike_chance, additional_base_critical_strike_chance,
			additional_critical_strike_chance_per_power_charge_permyriad,
			critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent_per_level, level,
			spell_critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent_per_power_charge, current_power_charges,
			virtual_global_cannot_crit,
			support_multiple_projectiles_critical_strike_chance_pluspercent_final, is_projectile,
			always_crit,
			global_critical_strike_chance_pluspercent_while_holding_staff, main_hand_weapon_type, global_critical_strike_chance_pluspercent_while_holding_bow,
			maximum_critical_strike_chance,
			unique_critical_strike_chance_pluspercent_final,
			critical_strike_chance_pluspercent_per_8_strength, strength,
			mine_critical_strike_chance_pluspercent, skill_is_mined,
			trap_critical_strike_chance_pluspercent, skill_is_trapped,
			critical_strike_chance_while_wielding_shield_pluspercent, off_hand_weapon_type,
			fire_critical_strike_chance_pluspercent, skill_is_fire_skill,
			lightning_critical_strike_chance_pluspercent, skill_is_lightning_skill,
			cold_critical_strike_chance_pluspercent, skill_is_cold_skill,
			elemental_critical_strike_chance_pluspercent,
			chaos_critical_strike_chance_pluspercent, skill_is_chaos_skill,
			vaal_skill_critical_strike_chance_pluspercent, skill_is_vaal_skill,
			totem_critical_strike_chance_pluspercent, skill_is_totemified,
			enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			assassinate_passive_critical_strike_chance_pluspercent_final,
			ambush_passive_critical_strike_chance_pluspercent_final,
			arrow_pierce_chance_applies_to_projectile_crit_chance, arrow_pierce_percent,
			global_critical_strike_chance_while_dual_wielding_pluspercent, is_dual_wielding )
		{
			//crit chance can be zero for a spell with no base crit chance - exception to the 5% - 95% range so curses don't display a crit chance
			if ( stats.GetStat( virtual_global_cannot_crit ) )
				return 0;

			const auto base_chance =
				stats.GetStat( base_spell_critical_strike_chance ) +
				stats.GetStat( additional_base_critical_strike_chance ) +
				stats.GetStat( additional_critical_strike_chance_per_power_charge_permyriad ) * stats.GetStat( current_power_charges );

			if ( base_chance <= 0 )
				return 0;

			if ( stats.GetStat( always_crit ) )
				return 10000;

			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const auto max_crit_override = stats.GetStat( maximum_critical_strike_chance );
			const bool is_elemental = stats.GetStat( skill_is_fire_skill ) || stats.GetStat( skill_is_lightning_skill ) || stats.GetStat( skill_is_cold_skill );

			unsigned crit_chance_increase = 100 + stats.GetStat( critical_strike_chance_pluspercent ) + stats.GetStat( spell_critical_strike_chance_pluspercent ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_level ) * stats.GetStat( level ) +
				stats.GetStat( current_power_charges ) * stats.GetStat( critical_strike_chance_pluspercent_per_power_charge ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_8_strength ) * stats.GetStat( strength ) / 8 +
				( main_hand_weapon_index == Items::Staff ? stats.GetStat( global_critical_strike_chance_pluspercent_while_holding_staff ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( global_critical_strike_chance_pluspercent_while_holding_bow ) : 0 ) +
				( stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_chance_pluspercent ) : 0 );

			// Trapped, totem and mined skills
			crit_chance_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_chance_pluspercent ) : 0;

			// Wielding shield
			crit_chance_increase += ( stats.GetStat( off_hand_weapon_type ) == Items::Shield ) ? stats.GetStat( critical_strike_chance_while_wielding_shield_pluspercent ) : 0;

			// Elemental increases
			crit_chance_increase += stats.GetStat( skill_is_fire_skill ) ? stats.GetStat( fire_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_lightning_skill ) ? stats.GetStat( lightning_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_cold_skill ) ? stats.GetStat( cold_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += is_elemental ? stats.GetStat( elemental_critical_strike_chance_pluspercent ) : 0;

			// Dual Wielding
			crit_chance_increase += stats.GetStat( is_dual_wielding ) ? stats.GetStat( global_critical_strike_chance_while_dual_wielding_pluspercent ) : 0;

			// Enchantment negative-positive buff.
			if ( !stats.GetStat( have_crit_in_past_4_seconds ) )
				crit_chance_increase += stats.GetStat( enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds );

			// Deadeye
			crit_chance_increase += ( stats.GetStat( is_projectile ) && stats.GetStat( arrow_pierce_chance_applies_to_projectile_crit_chance ) ) ?
				stats.GetStat( arrow_pierce_percent ) : 0;

			const float final_increase = 1.0f *
				( stats.GetStat( is_projectile ) ? Scale( 100 + stats.GetStat( support_multiple_projectiles_critical_strike_chance_pluspercent_final ) ) : 1.0f ) *
				Scale( 100 + stats.GetStat( unique_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( ambush_passive_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( assassinate_passive_critical_strike_chance_pluspercent_final ) );

			return Clamp( Round( base_chance * Scale( crit_chance_increase ) * final_increase ), 500, max_crit_override ? max_crit_override : 9500 );
		}

		VIRTUAL_STAT( secondary_critical_strike_chance,
			base_spell_critical_strike_chance, additional_base_critical_strike_chance,
			additional_critical_strike_chance_per_power_charge_permyriad,
			critical_strike_chance_pluspercent,
			critical_strike_chance_pluspercent_per_level, level,
			critical_strike_chance_pluspercent_per_power_charge, current_power_charges,
			virtual_global_cannot_crit,
			support_multiple_projectiles_critical_strike_chance_pluspercent_final, is_projectile,
			arrow_pierce_chance_applies_to_projectile_crit_chance, arrow_pierce_percent,
			always_crit,
			global_critical_strike_chance_pluspercent_while_holding_staff, main_hand_weapon_type, global_critical_strike_chance_pluspercent_while_holding_bow,
			maximum_critical_strike_chance,
			unique_critical_strike_chance_pluspercent_final,
			critical_strike_chance_pluspercent_per_8_strength, strength,
			mine_critical_strike_chance_pluspercent, skill_is_mined,
			trap_critical_strike_chance_pluspercent, skill_is_trapped,
			critical_strike_chance_while_wielding_shield_pluspercent, off_hand_weapon_type,
			fire_critical_strike_chance_pluspercent, skill_is_fire_skill,
			lightning_critical_strike_chance_pluspercent, skill_is_lightning_skill,
			cold_critical_strike_chance_pluspercent, skill_is_cold_skill,
			elemental_critical_strike_chance_pluspercent,
			chaos_critical_strike_chance_pluspercent, skill_is_chaos_skill,
			vaal_skill_critical_strike_chance_pluspercent, skill_is_vaal_skill,
			totem_critical_strike_chance_pluspercent, skill_is_totemified,
			enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			assassinate_passive_critical_strike_chance_pluspercent_final,
			ambush_passive_critical_strike_chance_pluspercent_final,		
			global_critical_strike_chance_while_dual_wielding_pluspercent, is_dual_wielding )
		{
			if ( stats.GetStat( virtual_global_cannot_crit ) )
				return 0;

			const auto base_chance =
				stats.GetStat( base_spell_critical_strike_chance ) +
				stats.GetStat( additional_base_critical_strike_chance ) +
				stats.GetStat( additional_critical_strike_chance_per_power_charge_permyriad ) * stats.GetStat( current_power_charges );

			if ( base_chance <= 0 )
				return 0;

			if ( stats.GetStat( always_crit ) )
				return 10000;

			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const auto max_crit_override = stats.GetStat( maximum_critical_strike_chance );
			const bool is_elemental = stats.GetStat( skill_is_fire_skill ) || stats.GetStat( skill_is_lightning_skill ) || stats.GetStat( skill_is_cold_skill );

			unsigned crit_chance_increase = 100 + stats.GetStat( critical_strike_chance_pluspercent ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_level ) * stats.GetStat( level ) +
				stats.GetStat( current_power_charges ) * stats.GetStat( critical_strike_chance_pluspercent_per_power_charge ) +
				stats.GetStat( critical_strike_chance_pluspercent_per_8_strength ) * stats.GetStat( strength ) / 8 +
				( main_hand_weapon_index == Items::Staff ? stats.GetStat( global_critical_strike_chance_pluspercent_while_holding_staff ) : 0 ) +
				( main_hand_weapon_index == Items::Bow ? stats.GetStat( global_critical_strike_chance_pluspercent_while_holding_bow ) : 0 ) +
				( stats.GetStat( skill_is_vaal_skill ) ? stats.GetStat( vaal_skill_critical_strike_chance_pluspercent ) : 0 );

			// Trapped, totem and mined skills
			crit_chance_increase += stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_critical_strike_chance_pluspercent ) : 0;

			// Wielding shield
			crit_chance_increase += ( off_hand_weapon_index == Items::Shield ? stats.GetStat( critical_strike_chance_while_wielding_shield_pluspercent ) : 0 );

			// Elemental increases
			crit_chance_increase += stats.GetStat( skill_is_fire_skill ) ? stats.GetStat( fire_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_lightning_skill ) ? stats.GetStat( lightning_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_cold_skill ) ? stats.GetStat( cold_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += stats.GetStat( skill_is_chaos_skill ) ? stats.GetStat( chaos_critical_strike_chance_pluspercent ) : 0;
			crit_chance_increase += is_elemental ? stats.GetStat( elemental_critical_strike_chance_pluspercent ) : 0;

			// Dual Wielding
			crit_chance_increase += stats.GetStat( is_dual_wielding ) ? stats.GetStat( global_critical_strike_chance_while_dual_wielding_pluspercent ) : 0;

			// Enchantment negative-positive buff.
			if( !stats.GetStat( have_crit_in_past_4_seconds ) )
				crit_chance_increase += stats.GetStat( enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds );

			// Deadeye
			crit_chance_increase += ( stats.GetStat( is_projectile ) && stats.GetStat( arrow_pierce_chance_applies_to_projectile_crit_chance ) ) ?
				stats.GetStat( arrow_pierce_percent ) : 0;

			const float final_increase = 1.0f *
				( stats.GetStat( is_projectile ) ? Scale( 100 + stats.GetStat( support_multiple_projectiles_critical_strike_chance_pluspercent_final ) ) : 1.0f ) *
				Scale( 100 + stats.GetStat( unique_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( ambush_passive_critical_strike_chance_pluspercent_final ) ) *
				Scale( 100 + stats.GetStat( assassinate_passive_critical_strike_chance_pluspercent_final ) );

			return Clamp( Round( base_chance * Scale( crit_chance_increase ) * final_increase ), 500, max_crit_override ? max_crit_override : 9500 );
		}

		VIRTUAL_STAT( virtual_global_cannot_crit,
			global_cannot_crit,
			resolute_technique )
		{
			return ( stats.GetStat( global_cannot_crit ) || stats.GetStat( resolute_technique ) );
		}

		VIRTUAL_STAT( strength,
			base_strength,
			base_strength_and_dexterity,
			base_strength_and_intelligence,
			additional_strength,
			additional_strength_and_intelligence,
			additional_strength_and_dexterity,
			additional_all_attributes,
			strength_per_level, level,
			strength_pluspercent,
			all_attributes_pluspercent_per_assigned_keystone, number_of_assigned_keystones,
			all_attributes_pluspercent )
		{
			return std::max( 0, Round( ( stats.GetStat( base_strength ) +
				stats.GetStat( base_strength_and_dexterity ) +
				stats.GetStat( base_strength_and_intelligence ) +
				stats.GetStat( additional_strength ) +
				stats.GetStat( additional_strength_and_intelligence ) +
				stats.GetStat( additional_strength_and_dexterity ) +
				stats.GetStat( additional_all_attributes ) +
				stats.GetStat( strength_per_level ) *
				( stats.GetStat( level ) - 1 ) ) *
				( 100 + stats.GetStat( strength_pluspercent ) +
					stats.GetStat( all_attributes_pluspercent ) +
					stats.GetStat( all_attributes_pluspercent_per_assigned_keystone ) * stats.GetStat( number_of_assigned_keystones ) ) / 100.0f ) );
		}

		VIRTUAL_STAT( dexterity,
			base_dexterity,
			base_strength_and_dexterity,
			base_dexterity_and_intelligence,
			additional_dexterity,
			additional_strength_and_dexterity,
			additional_dexterity_and_intelligence,
			additional_all_attributes,
			dexterity_per_level, level,
			dexterity_pluspercent,
			all_attributes_pluspercent_per_assigned_keystone, number_of_assigned_keystones,
			all_attributes_pluspercent )
		{
			return std::max( 0, Round( ( stats.GetStat( base_dexterity ) +
				stats.GetStat( base_strength_and_dexterity ) +
				stats.GetStat( base_dexterity_and_intelligence ) +
				stats.GetStat( additional_dexterity ) +
				stats.GetStat( additional_strength_and_dexterity ) +
				stats.GetStat( additional_dexterity_and_intelligence ) +
				stats.GetStat( additional_all_attributes ) +
				stats.GetStat( dexterity_per_level ) *
				( stats.GetStat( level ) - 1 ) ) *
				( 100 + stats.GetStat( dexterity_pluspercent ) +
					stats.GetStat( all_attributes_pluspercent ) +
					stats.GetStat( all_attributes_pluspercent_per_assigned_keystone ) * stats.GetStat( number_of_assigned_keystones ) ) / 100.0f ) );
		}

		VIRTUAL_STAT( intelligence,
			base_intelligence,
			base_strength_and_intelligence,
			base_dexterity_and_intelligence,
			additional_intelligence,
			additional_strength_and_intelligence,
			additional_dexterity_and_intelligence,
			additional_all_attributes,
			intelligence_per_level, level,
			intelligence_pluspercent,
			intelligence_pluspercent_per_equipped_unique, number_of_equipped_uniques,
			all_attributes_pluspercent_per_assigned_keystone, number_of_assigned_keystones,
			all_attributes_pluspercent )
		{
			return std::max( 0, Round( ( stats.GetStat( base_intelligence ) +
				stats.GetStat( base_strength_and_intelligence ) +
				stats.GetStat( base_dexterity_and_intelligence ) +
				stats.GetStat( additional_intelligence ) +
				stats.GetStat( additional_strength_and_intelligence ) +
				stats.GetStat( additional_dexterity_and_intelligence ) +
				stats.GetStat( additional_all_attributes ) +
				stats.GetStat( intelligence_per_level ) *
				( stats.GetStat( level ) - 1 ) ) *
				( 100 + stats.GetStat( intelligence_pluspercent ) +
					stats.GetStat( all_attributes_pluspercent ) +
					stats.GetStat( intelligence_pluspercent_per_equipped_unique ) * stats.GetStat( number_of_equipped_uniques ) +
					stats.GetStat( all_attributes_pluspercent_per_assigned_keystone ) * stats.GetStat( number_of_assigned_keystones ) ) / 100.0f ) );
		}

		VIRTUAL_STAT( virtual_penetrate_elemental_resistances_percent
			, base_penetrate_elemental_resistances_percent
			, reduce_enemy_elemental_resistance_percent
			, trap_damage_penetrates_percent_elemental_resistance, skill_is_trapped
			, mine_damage_penetrates_percent_elemental_resistance, skill_is_mined
			, enchantment_boots_damage_penetrates_elemental_resistance_percent_while_you_havent_killed_for_4_seconds, have_killed_in_past_4_seconds
			, penetrate_elemental_resistance_per_frenzy_charge_percent, current_frenzy_charges
			)
		{
			return stats.GetStat( base_penetrate_elemental_resistances_percent ) + stats.GetStat( reduce_enemy_elemental_resistance_percent ) +
				( stats.GetStat( have_killed_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_damage_penetrates_elemental_resistance_percent_while_you_havent_killed_for_4_seconds ) ) +
				( stats.GetStat( skill_is_trapped ) ? stats.GetStat( trap_damage_penetrates_percent_elemental_resistance ) : 0 ) +
				( stats.GetStat( skill_is_mined ) ? stats.GetStat( mine_damage_penetrates_percent_elemental_resistance ) : 0 ) +
				( stats.GetStat( penetrate_elemental_resistance_per_frenzy_charge_percent ) * stats.GetStat( current_frenzy_charges ) );
		}

		VIRTUAL_STAT( reduce_enemy_fire_resistance_percent,
			base_reduce_enemy_fire_resistance_percent,
			virtual_penetrate_elemental_resistances_percent )
		{
			return stats.GetStat( base_reduce_enemy_fire_resistance_percent ) +
				stats.GetStat( virtual_penetrate_elemental_resistances_percent );
		}

		VIRTUAL_STAT( reduce_enemy_cold_resistance_percent,
			base_reduce_enemy_cold_resistance_percent,
			virtual_penetrate_elemental_resistances_percent )
		{
			return stats.GetStat( base_reduce_enemy_cold_resistance_percent ) +
				stats.GetStat( virtual_penetrate_elemental_resistances_percent );
		}

		VIRTUAL_STAT( reduce_enemy_lightning_resistance_percent,
			base_reduce_enemy_lightning_resistance_percent,
			virtual_penetrate_elemental_resistances_percent )
		{
			return stats.GetStat( base_reduce_enemy_lightning_resistance_percent ) +
				stats.GetStat( virtual_penetrate_elemental_resistances_percent );
		}

		VIRTUAL_STAT( main_hand_critical_strike_multiplier
			, base_critical_strike_multiplier
			, main_hand_critical_strike_multiplier_plus
			, no_critical_strike_multiplier
			, keystone_elemental_overload
			)
		{
			if ( stats.GetStat( no_critical_strike_multiplier ) || stats.GetStat( keystone_elemental_overload ) )
				return 100;

			return std::max( 100, stats.GetStat( base_critical_strike_multiplier ) + stats.GetStat( main_hand_critical_strike_multiplier_plus ) );
		}

		VIRTUAL_STAT( off_hand_critical_strike_multiplier
			, base_critical_strike_multiplier
			, off_hand_critical_strike_multiplier_plus
			, no_critical_strike_multiplier
			, keystone_elemental_overload
			)
		{
			if ( stats.GetStat( no_critical_strike_multiplier ) || stats.GetStat( keystone_elemental_overload ) )
				return 100;

			return std::max( 100, stats.GetStat( base_critical_strike_multiplier ) + stats.GetStat( off_hand_critical_strike_multiplier_plus ) );
		}

		VIRTUAL_STAT( spell_critical_strike_multiplier
			, base_critical_strike_multiplier
			, spell_critical_strike_multiplier_plus
			, no_critical_strike_multiplier
			, keystone_elemental_overload
			)
		{
			if ( stats.GetStat( no_critical_strike_multiplier ) || stats.GetStat( keystone_elemental_overload ) )
				return 100;

			return std::max( 100, stats.GetStat( base_critical_strike_multiplier ) + stats.GetStat( spell_critical_strike_multiplier_plus ) );
		}

		VIRTUAL_STAT( secondary_critical_strike_multiplier
			, base_critical_strike_multiplier
			, secondary_damage_critical_strike_multiplier_plus
			, no_critical_strike_multiplier
			, keystone_elemental_overload
			)
		{
			if ( stats.GetStat( no_critical_strike_multiplier ) || stats.GetStat( keystone_elemental_overload ) )
				return 100;

			return std::max( 100, stats.GetStat( base_critical_strike_multiplier ) + stats.GetStat( secondary_damage_critical_strike_multiplier_plus ) );
		}

		VIRTUAL_STAT( avoid_chill_percent_while_have_onslaught,
			base_avoid_chill_percent_while_have_onslaught,
			avoid_freeze_chill_ignite_percent_while_have_onslaught )
		{
			return stats.GetStat( base_avoid_chill_percent_while_have_onslaught ) +
				stats.GetStat( avoid_freeze_chill_ignite_percent_while_have_onslaught );
		}

		VIRTUAL_STAT( avoid_freeze_percent_while_have_onslaught,
			base_avoid_freeze_percent_while_have_onslaught,
			avoid_freeze_chill_ignite_percent_while_have_onslaught )
		{
			return stats.GetStat( base_avoid_freeze_percent_while_have_onslaught ) +
				stats.GetStat( avoid_freeze_chill_ignite_percent_while_have_onslaught );
		}

		VIRTUAL_STAT( avoid_shock_percent_while_have_onslaught,
			base_avoid_shock_percent_while_have_onslaught )
		{
			return stats.GetStat( base_avoid_shock_percent_while_have_onslaught );
		}

		VIRTUAL_STAT( avoid_ignite_percent_while_have_onslaught,
			base_avoid_ignite_percent_while_have_onslaught,
			avoid_freeze_chill_ignite_percent_while_have_onslaught )
		{
			return stats.GetStat( base_avoid_ignite_percent_while_have_onslaught ) +
				stats.GetStat( avoid_freeze_chill_ignite_percent_while_have_onslaught );
		}

		VIRTUAL_STAT( avoid_chill_percent
			, base_avoid_chill_percent
			, avoid_chill_percent_while_have_onslaught, virtual_has_onslaught
			, avoid_freeze_chill_ignite_percent_with_her_blessing, have_her_blessing
			, avoid_all_elemental_status_percent
			, avoid_ailments_percent_on_consecrated_ground, on_consecrated_ground
			, avoid_status_ailments_percent_during_flask_effect, using_flask
			, immune_to_chill )
		{
			if( !!stats.GetStat( immune_to_chill ) )
				return 100;

			return stats.GetStat( base_avoid_chill_percent ) + stats.GetStat( avoid_all_elemental_status_percent ) +
				( stats.GetStat( have_her_blessing ) ? stats.GetStat( avoid_freeze_chill_ignite_percent_with_her_blessing ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( avoid_ailments_percent_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? stats.GetStat( avoid_chill_percent_while_have_onslaught ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( avoid_status_ailments_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( immune_to_chill
			, base_immune_to_chill
			, immune_to_status_ailments_while_phased, virtual_phase_through_objects
			, immune_to_elemental_status_ailments_during_flask_effect, using_flask
			)
		{
			return stats.GetStat( base_immune_to_chill ) ||
				( stats.GetStat( immune_to_status_ailments_while_phased ) && stats.GetStat( virtual_phase_through_objects ) ) ||
				( stats.GetStat( using_flask ) && stats.GetStat( immune_to_elemental_status_ailments_during_flask_effect ) );
		}

		VIRTUAL_STAT( avoid_freeze_percent,
			base_avoid_freeze_percent,
			avoid_freeze_percent_while_have_onslaught, virtual_has_onslaught,
			avoid_freeze_chill_ignite_percent_with_her_blessing, have_her_blessing,
			avoid_all_elemental_status_percent,
			avoid_ailments_percent_on_consecrated_ground, on_consecrated_ground,
			avoid_status_ailments_percent_during_flask_effect, using_flask,
			avoid_freeze_shock_ignite_bleed_percent_during_flask_effect,
			immune_to_freeze )
		{
			if( !!stats.GetStat( immune_to_freeze ) )
				return 100;

			return stats.GetStat( base_avoid_freeze_percent ) + stats.GetStat( avoid_all_elemental_status_percent ) +
				( stats.GetStat( have_her_blessing ) ? stats.GetStat( avoid_freeze_chill_ignite_percent_with_her_blessing ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( avoid_ailments_percent_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? stats.GetStat( avoid_freeze_percent_while_have_onslaught ) : 0 ) +
				( stats.GetStat( using_flask ) ?
					stats.GetStat( avoid_status_ailments_percent_during_flask_effect ) +
					stats.GetStat( avoid_freeze_shock_ignite_bleed_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( immune_to_freeze
			, base_immune_to_freeze
			, immune_to_status_ailments_while_phased, virtual_phase_through_objects
			, immune_to_elemental_status_ailments_during_flask_effect, using_flask
			)
		{
			return stats.GetStat( base_immune_to_freeze ) ||
				( stats.GetStat( immune_to_status_ailments_while_phased ) && stats.GetStat( virtual_phase_through_objects ) ) ||
				( stats.GetStat( using_flask ) && stats.GetStat( immune_to_elemental_status_ailments_during_flask_effect ) );
		}

		VIRTUAL_STAT( avoid_shock_percent
			, base_avoid_shock_percent
			, avoid_shock_percent_while_have_onslaught, virtual_has_onslaught
			, avoid_all_elemental_status_percent
			, avoid_ailments_percent_on_consecrated_ground, on_consecrated_ground
			, avoid_status_ailments_percent_during_flask_effect, using_flask
			, avoid_freeze_shock_ignite_bleed_percent_during_flask_effect
			, cannot_be_shocked )
		{
			if( !!stats.GetStat( cannot_be_shocked ) )
				return 100;

			return stats.GetStat( base_avoid_shock_percent ) + stats.GetStat( avoid_all_elemental_status_percent ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( avoid_ailments_percent_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? stats.GetStat( avoid_shock_percent_while_have_onslaught ) : 0 ) +
				( stats.GetStat( using_flask ) ?
					stats.GetStat( avoid_status_ailments_percent_during_flask_effect ) +
					stats.GetStat( avoid_freeze_shock_ignite_bleed_percent_during_flask_effect ) : 0 );
		}

		//Currently, only shock needs a "cannot be" virtual stat. If adding these for the others in future, make sure that the correct order is maintained.
		// "Immune to" stats should cause "Cannot be" stats to be true, but not the other way around (because immune also means remove it when gaining the stat, but cannot be doesn't).
		// "Cannot be" stats then feed into the "avoid" stats and set them to 100%, so that we only need to check the avoid stat to see if we can add the debuff, rather than separately
		// also checking "cannot be" or "immune" stats.
		VIRTUAL_STAT( cannot_be_shocked
			, immune_to_shock
			, cannot_be_shocked_while_frozen, is_frozen
			, cannot_be_shocked_while_at_maximum_endurance_charges, virtual_maximum_endurance_charges, current_endurance_charges
			)
		{
			return stats.GetStat( immune_to_shock ) ||
				( stats.GetStat( cannot_be_shocked_while_frozen ) && stats.GetStat( is_frozen ) ) ||
				( stats.GetStat( cannot_be_shocked_while_at_maximum_endurance_charges ) && stats.GetStat( virtual_maximum_endurance_charges ) == stats.GetStat( current_endurance_charges ) );
		}

		VIRTUAL_STAT( immune_to_shock
			, base_immune_to_shock
			, immune_to_status_ailments_while_phased, virtual_phase_through_objects
			, immune_to_elemental_status_ailments_during_flask_effect, using_flask
			)
		{
			return stats.GetStat( base_immune_to_shock ) ||
				( stats.GetStat( immune_to_status_ailments_while_phased ) && stats.GetStat( virtual_phase_through_objects ) ) ||
				( stats.GetStat( using_flask ) && stats.GetStat( immune_to_elemental_status_ailments_during_flask_effect ) );
		}

		VIRTUAL_STAT( avoid_ignite_percent
			, base_avoid_ignite_percent
			, avoid_ignite_percent_while_have_onslaught, virtual_has_onslaught
			, avoid_freeze_chill_ignite_percent_with_her_blessing, have_her_blessing
			, avoid_all_elemental_status_percent
			, avoid_ignite_percent_when_on_low_life, on_low_life
			, avoid_ailments_percent_on_consecrated_ground, on_consecrated_ground
			, avoid_status_ailments_percent_during_flask_effect, using_flask
			, avoid_freeze_shock_ignite_bleed_percent_during_flask_effect
			, immune_to_ignite )
		{
			if( stats.GetStat( immune_to_shock ) )
				return 100;

			return stats.GetStat( base_avoid_ignite_percent ) + stats.GetStat( avoid_all_elemental_status_percent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( avoid_ignite_percent_when_on_low_life ) : 0 ) +
				( stats.GetStat( have_her_blessing ) ? stats.GetStat( avoid_freeze_chill_ignite_percent_with_her_blessing ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( avoid_ailments_percent_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( virtual_has_onslaught ) ? stats.GetStat( avoid_ignite_percent_while_have_onslaught ) : 0 ) +
				( stats.GetStat( using_flask ) ?
					stats.GetStat( avoid_status_ailments_percent_during_flask_effect ) +
					stats.GetStat( avoid_freeze_shock_ignite_bleed_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( immune_to_ignite
			, base_immune_to_ignite
			, immune_to_status_ailments_while_phased, virtual_phase_through_objects
			, immune_to_elemental_status_ailments_during_flask_effect, using_flask
			)
		{
			return stats.GetStat( base_immune_to_ignite ) ||
				( stats.GetStat( immune_to_status_ailments_while_phased ) && stats.GetStat( virtual_phase_through_objects ) ) ||
				( stats.GetStat( using_flask ) && stats.GetStat( immune_to_elemental_status_ailments_during_flask_effect ) );
		}

		VIRTUAL_STAT( avoid_bleed_percent
			, base_avoid_bleed_percent
			, using_flask
			, avoid_freeze_shock_ignite_bleed_percent_during_flask_effect
			, base_cannot_gain_bleeding
			, virtual_immune_to_bleeding
			)
		{
			if( stats.GetStat( base_cannot_gain_bleeding ) || stats.GetStat( virtual_immune_to_bleeding ) ) //if we get more conditional cannot gain bleeding stats, this will need to be it's own virtual stat - see note on cannot_be_shocked, above
				return 100;

			return stats.GetStat( base_avoid_bleed_percent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( avoid_freeze_shock_ignite_bleed_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( virtual_immune_to_bleeding,
			immune_to_bleeding,
			immune_to_bleeding_while_leeching, is_leeching )
		{
			return stats.GetStat( immune_to_bleeding ) ||
				( stats.GetStat( immune_to_bleeding_while_leeching ) && stats.GetStat( is_leeching ) );
		}

		VIRTUAL_STAT( self_chill_duration_minuspercent,
			base_self_chill_duration_minuspercent,
			self_elemental_status_duration_minuspercent )
		{
			return stats.GetStat( base_self_chill_duration_minuspercent ) + stats.GetStat( self_elemental_status_duration_minuspercent );
		}

		VIRTUAL_STAT( self_freeze_duration_minuspercent,
			base_self_freeze_duration_minuspercent,
			self_elemental_status_duration_minuspercent )
		{
			return stats.GetStat( base_self_freeze_duration_minuspercent ) + stats.GetStat( self_elemental_status_duration_minuspercent );
		}

		VIRTUAL_STAT( self_shock_duration_minuspercent,
			base_self_shock_duration_minuspercent,
			self_elemental_status_duration_minuspercent )
		{
			return stats.GetStat( base_self_shock_duration_minuspercent ) + stats.GetStat( self_elemental_status_duration_minuspercent );
		}

		VIRTUAL_STAT( self_ignite_duration_minuspercent,
			base_self_ignite_duration_minuspercent,
			self_elemental_status_duration_minuspercent )
		{
			return stats.GetStat( base_self_ignite_duration_minuspercent ) + stats.GetStat( self_elemental_status_duration_minuspercent );
		}

		VIRTUAL_STAT( display_skill_increased_item_quantity_pluspercent,
			item_found_quantity_pluspercent,
			killed_monster_dropped_item_quantity_pluspercent )
		{
			return stats.GetStat( item_found_quantity_pluspercent ) + stats.GetStat( killed_monster_dropped_item_quantity_pluspercent );
		}

		VIRTUAL_STAT( display_skill_increased_item_rarity_pluspercent,
			item_found_rarity_pluspercent,
			killed_monster_dropped_item_rarity_pluspercent )
		{
			return stats.GetStat( item_found_rarity_pluspercent ) + stats.GetStat( killed_monster_dropped_item_rarity_pluspercent );
		}

		VIRTUAL_STAT( character_sheet_monster_level, level )
		{
			return std::min( stats.GetStat( level ), 80 );
		}

		VIRTUAL_STAT( chance_to_hit_percent,
			character_sheet_monster_level,
			main_hand_accuracy_rating,
			off_hand_accuracy_rating,
			is_dual_wielding,
			casting_spell,
			main_hand_always_hit,
			off_hand_always_hit,
			blinded )
		{
			if ( stats.GetStat( casting_spell ) )
				return 0;

			const unsigned reference_evasion = CalculateExpectedEvasionForLevel( stats.GetStat( character_sheet_monster_level ) );

			const auto main_hand_hit_chance = ( stats.GetStat( main_hand_always_hit ) ? 1.0f :
				CalculateHitChance( stats.GetStat( main_hand_accuracy_rating ), reference_evasion, !!stats.GetStat( blinded ) ) );

			if ( !stats.GetStat( is_dual_wielding ) )
				return Round( main_hand_hit_chance * 100.0f );

			const auto off_hand_hit_chance = ( stats.GetStat( off_hand_always_hit ) ? 1.0f :
				CalculateHitChance( stats.GetStat( off_hand_accuracy_rating ), reference_evasion, !!stats.GetStat( blinded ) ) );

			return Round( ( main_hand_hit_chance + off_hand_hit_chance ) / 2.0f * 100.0f );
		}

		VIRTUAL_STAT( cannot_evade,
			base_cannot_evade,
			keystone_unwavering_stance )
		{
			return stats.GetStat( base_cannot_evade ) || stats.GetStat( keystone_unwavering_stance );
		}

		VIRTUAL_STAT( chance_to_evade_percent,
			character_sheet_monster_level,
			evasion_rating,
			cannot_evade )
		{
			if ( stats.GetStat( cannot_evade ) )
				return 0;

			const unsigned reference_accuracy = CalculateExpectedAccuracyForLevel( stats.GetStat( character_sheet_monster_level ) );

			return Round( ( 1.0f - CalculateHitChance( reference_accuracy, stats.GetStat( evasion_rating ), false ) ) * 100.0f );
		}

		VIRTUAL_STAT( display_estimated_physical_damage_reduciton_percent,
			character_sheet_monster_level,
			physical_damage_reduction_rating,
			additional_physical_damage_reduction_percent,
			maximum_physical_damage_reduction_percent )
		{
			const unsigned reference_damage = CalculateExpectedDamageForLevel( stats.GetStat( character_sheet_monster_level ) );

			return Round( CalculateReduction( stats.GetStat( physical_damage_reduction_rating ),
				stats.GetStat( additional_physical_damage_reduction_percent ),
				stats.GetStat( maximum_physical_damage_reduction_percent ),
				reference_damage ) * 100.0f );
		}

		VIRTUAL_STAT( display_estimated_physical_damage_reduciton_against_projectiles_percent,
			character_sheet_monster_level,
			physical_damage_reduction_rating_against_projectiles,
			additional_physical_damage_reduction_percent,
			maximum_physical_damage_reduction_percent )
		{
			const unsigned reference_damage = CalculateExpectedDamageForLevel( stats.GetStat( character_sheet_monster_level ) );

			return Round( CalculateReduction( stats.GetStat( physical_damage_reduction_rating_against_projectiles ),
				stats.GetStat( additional_physical_damage_reduction_percent ),
				stats.GetStat( maximum_physical_damage_reduction_percent ),
				reference_damage ) * 100.0f );
		}

		VIRTUAL_STAT( main_hand_attack_duration_ms,
			main_hand_base_weapon_attack_duration_ms,
			main_hand_attack_speed_pluspercent,
			cast_time_overrides_attack_duration,
			base_spell_cast_time_ms )
		{
			const float ias = 1.0f / ( 1.0f + stats.GetStat( main_hand_attack_speed_pluspercent ) / 100.0f );
			const auto duration = stats.GetStat( cast_time_overrides_attack_duration ) ? stats.GetStat( base_spell_cast_time_ms ) : stats.GetStat( main_hand_base_weapon_attack_duration_ms );
			return Round( duration * ias );
		}

		VIRTUAL_STAT( off_hand_attack_duration_ms,
			off_hand_base_weapon_attack_duration_ms,
			off_hand_attack_speed_pluspercent,
			cast_time_overrides_attack_duration,
			base_spell_cast_time_ms )
		{
			const float ias = Scale( 100 + stats.GetStat( off_hand_attack_speed_pluspercent ) );
			const auto duration = stats.GetStat( cast_time_overrides_attack_duration ) ? stats.GetStat( base_spell_cast_time_ms ) : stats.GetStat( off_hand_base_weapon_attack_duration_ms );
			return Round( duration / ias );
		}

		VIRTUAL_STAT( hundred_times_attacks_per_second,
			main_hand_attack_duration_ms,
			off_hand_attack_duration_ms,
			is_dual_wielding,
			casting_spell )
		{
			if ( stats.GetStat( casting_spell ) )
				return 0;

			const auto main_hand_attacks_per_second = 1000.f / stats.GetStat( main_hand_attack_duration_ms );

			if ( stats.GetStat( is_dual_wielding ) )
			{
				const auto off_hand_attacks_per_second = 1000.f / stats.GetStat( off_hand_attack_duration_ms );

				const auto average = ( main_hand_attacks_per_second + off_hand_attacks_per_second ) / 2.f;
				return Round( average * 100.f );
			}
			else
				return Round( main_hand_attacks_per_second * 100.0f );
		}

		//this won't factor in curse/aura cast speed bonuses, but is only used for DPS display/damage scaling, so those don't apply anyway
		VIRTUAL_STAT( spell_cast_time_ms,
			base_spell_cast_time_ms,
			cast_speed_pluspercent )
		{
			const auto base = stats.GetStat( base_spell_cast_time_ms );
			const auto increase = Scale( 100 + stats.GetStat( cast_speed_pluspercent ) );
			return Round( base / increase );
		}

		VIRTUAL_STAT( hundred_times_casts_per_second,
			casting_spell,
			spell_cast_time_ms )
		{
			if ( !stats.GetStat( casting_spell ) )
				return 0;

			return Round( ( 1000.0f / stats.GetStat( spell_cast_time_ms ) ) * 100.0f );
		}

		VIRTUAL_STAT( hundred_times_non_spell_casts_per_second,
			base_spell_cast_time_ms )
		{
			return Round( ( 1000.0f / stats.GetStat( base_spell_cast_time_ms ) ) * 100.0f );
		}

		VIRTUAL_STAT( skill_number_of_additional_hits,
			base_skill_number_of_additional_hits,
			skill_double_hits_when_dual_wielding,
			is_dual_wielding )
		{
			if ( stats.GetStat( skill_double_hits_when_dual_wielding ) && stats.GetStat( is_dual_wielding ) )
				return 2 * ( 1 + stats.GetStat( base_skill_number_of_additional_hits ) ) - 1;

			return stats.GetStat( base_skill_number_of_additional_hits );
		}

		VIRTUAL_STAT( hundred_times_average_damage_per_hit,
			main_hand_minimum_total_damage,
			main_hand_maximum_total_damage,
			off_hand_minimum_total_damage,
			off_hand_maximum_total_damage,
			spell_minimum_total_damage,
			spell_maximum_total_damage,
			secondary_minimum_total_damage,
			secondary_maximum_total_damage,
			casting_spell, display_skill_deals_secondary_damage, is_dual_wielding,
			main_hand_critical_strike_chance,
			off_hand_critical_strike_chance,
			spell_critical_strike_chance,
			secondary_critical_strike_chance,
			main_hand_critical_strike_multiplier,
			off_hand_critical_strike_multiplier,
			spell_critical_strike_multiplier,
			secondary_critical_strike_multiplier,
			chance_to_hit_percent,
			non_critical_damage_multiplier_pluspercent )
		{
			float average_damage;
			float crit_chance;
			float average_damage_on_crit;
			int chance_to_hit;

			if ( stats.GetStat( display_skill_deals_secondary_damage ) )
			{
				chance_to_hit = 100;
				average_damage = ( stats.GetStat( secondary_minimum_total_damage ) + stats.GetStat( secondary_maximum_total_damage ) ) / 2.0f;
				crit_chance = stats.GetStat( secondary_critical_strike_chance ) / 10000.0f;
				average_damage_on_crit = average_damage * stats.GetStat( secondary_critical_strike_multiplier ) / 100.0f;
			}
			else if ( stats.GetStat( casting_spell ) )
			{
				chance_to_hit = 100;
				average_damage = ( stats.GetStat( spell_minimum_total_damage ) + stats.GetStat( spell_maximum_total_damage ) ) / 2.0f;
				crit_chance = stats.GetStat( spell_critical_strike_chance ) / 10000.0f;
				average_damage_on_crit = average_damage * stats.GetStat( spell_critical_strike_multiplier ) / 100.0f;

			}
			else
			{
				chance_to_hit = stats.GetStat( chance_to_hit_percent );
				average_damage = ( stats.GetStat( main_hand_minimum_total_damage ) + stats.GetStat( main_hand_maximum_total_damage ) ) / 2.0f;
				crit_chance = stats.GetStat( main_hand_critical_strike_chance ) / 10000.0f;
				average_damage_on_crit = average_damage * stats.GetStat( main_hand_critical_strike_multiplier ) / 100.0f;

				if ( stats.GetStat( is_dual_wielding ) )
				{
					const auto off_hand_average_damage = ( stats.GetStat( off_hand_minimum_total_damage ) + stats.GetStat( off_hand_maximum_total_damage ) ) / 2.0f;
					const auto off_hand_crit_chance = stats.GetStat( off_hand_critical_strike_chance ) / 10000.0f;
					const auto off_hand_average_damage_on_crit = off_hand_average_damage * stats.GetStat( off_hand_critical_strike_multiplier ) / 100.0f;

					return Round( (
						( ( 100 + stats.GetStat( non_critical_damage_multiplier_pluspercent ) ) * average_damage / 100 ) * ( 1.0f - crit_chance ) +
						( ( 100 + stats.GetStat( non_critical_damage_multiplier_pluspercent ) ) * off_hand_average_damage / 100 ) * ( 1.0f - off_hand_crit_chance ) +
						average_damage_on_crit * crit_chance +
						off_hand_average_damage_on_crit * off_hand_crit_chance ) *
						chance_to_hit / 2 );
				}
			}
			return Round( (
				( ( 100 + stats.GetStat( non_critical_damage_multiplier_pluspercent ) ) * average_damage / 100 ) * ( 1.0f - crit_chance ) +
				average_damage_on_crit * crit_chance ) *
				chance_to_hit );

		}

		VIRTUAL_STAT( hundred_times_average_damage_per_skill_use,
			hundred_times_average_damage_per_hit,
			skill_number_of_additional_hits )
		{
			return stats.GetStat( hundred_times_average_damage_per_hit ) * ( 1 + stats.GetStat( skill_number_of_additional_hits ) );
		}

		VIRTUAL_STAT( skill_show_average_damage_instead_of_dps,
			base_skill_show_average_damage_instead_of_dps,
			spell_uncastable_if_triggerable, casting_spell )
		{
			return stats.GetStat( base_skill_show_average_damage_instead_of_dps ) || ( stats.GetStat( spell_uncastable_if_triggerable ) && stats.GetStat( casting_spell ) );
		}

		VIRTUAL_STAT( display_hundred_times_damage_per_skill_use,
			hundred_times_average_damage_per_skill_use, skill_show_average_damage_instead_of_dps )
		{
			return stats.GetStat( skill_show_average_damage_instead_of_dps ) ? stats.GetStat( hundred_times_average_damage_per_skill_use ) : 0;
		}

		VIRTUAL_STAT( hundred_times_damage_per_second,
			hundred_times_average_damage_per_skill_use,
			skill_show_average_damage_instead_of_dps,
			hundred_times_attacks_per_second,
			hundred_times_casts_per_second,
			casting_spell )
		{
			if ( stats.GetStat( skill_show_average_damage_instead_of_dps ) )
				return 0;

			if ( stats.GetStat( casting_spell ) )
			{
				return Round( stats.GetStat( hundred_times_casts_per_second ) / 100.0f * stats.GetStat( hundred_times_average_damage_per_skill_use ) );
			}
			else
			{
				return Round( stats.GetStat( hundred_times_attacks_per_second ) / 100.0f * stats.GetStat( hundred_times_average_damage_per_skill_use ) );
			}
		}

		VIRTUAL_STAT( override_pvp_scaling_time_ms,
			skill_override_pvp_scaling_time_ms,
			trap_override_pvp_scaling_time_ms,
			mine_override_pvp_scaling_time_ms )
		{
			if ( const auto time = stats.GetStat( skill_override_pvp_scaling_time_ms ) )
				return time;

			if ( const auto time = stats.GetStat( trap_override_pvp_scaling_time_ms ) )
				return time;

			if ( const auto time = stats.GetStat( mine_override_pvp_scaling_time_ms ) )
				return time;

			return 0;
		}

		VIRTUAL_STAT( main_hand_hit_causes_monster_flee_percent,
			global_hit_causes_monster_flee_percent,
			main_hand_local_hit_causes_monster_flee_percent )
		{
			return stats.GetStat( main_hand_local_hit_causes_monster_flee_percent ) + stats.GetStat( global_hit_causes_monster_flee_percent );
		}

		VIRTUAL_STAT( off_hand_hit_causes_monster_flee_percent,
			global_hit_causes_monster_flee_percent,
			off_hand_local_hit_causes_monster_flee_percent )
		{
			return stats.GetStat( off_hand_local_hit_causes_monster_flee_percent ) + stats.GetStat( global_hit_causes_monster_flee_percent );
		}

		VIRTUAL_STAT( virtual_global_attacks_always_hit,
			global_always_hit,
			resolute_technique )
		{
			return ( stats.GetStat( global_always_hit ) || stats.GetStat( resolute_technique ) );
		}

		VIRTUAL_STAT( main_hand_always_hit,
			virtual_global_attacks_always_hit,
			main_hand_local_always_hit )
		{
			return stats.GetStat( virtual_global_attacks_always_hit ) || stats.GetStat( main_hand_local_always_hit );
		}

		VIRTUAL_STAT( off_hand_always_hit,
			virtual_global_attacks_always_hit,
			off_hand_local_always_hit )
		{
			return stats.GetStat( virtual_global_attacks_always_hit ) || stats.GetStat( off_hand_local_always_hit );
		}

		VIRTUAL_STAT( total_number_of_projectiles_to_fire,
			virtual_number_of_additional_projectiles,
			base_is_projectile,
			main_hand_weapon_type,
			show_number_of_projectiles,
			skill_can_fire_wand_projectiles,
			base_number_of_projectiles_in_spiral_nova )
		{
			//if not a projectile skill (or explicitly showing the number - used by lightning strike)  and not going to be fired as a projectile (e.g. elemental hit from a wand), return 0
			if ( !( stats.GetStat( base_is_projectile ) || stats.GetStat( show_number_of_projectiles ) ) && ( stats.GetStat( main_hand_weapon_type ) != Items::Wand || !stats.GetStat( skill_can_fire_wand_projectiles ) ) )
				return 0;

			const auto base = stats.GetStat( base_number_of_projectiles_in_spiral_nova ) ? stats.GetStat( base_number_of_projectiles_in_spiral_nova ) : 1;
			return stats.GetStat( virtual_number_of_additional_projectiles ) + base;
		}

		VIRTUAL_STAT( total_number_of_arrows_to_fire,
			number_of_additional_arrows,
			is_projectile,
			main_hand_weapon_type,
			skill_can_fire_arrows,
			base_number_of_projectiles_in_spiral_nova )
		{
			//if not using a bow for this skill, return 0
			if ( stats.GetStat( main_hand_weapon_type ) != Items::Bow || !stats.GetStat( skill_can_fire_arrows ) )
				return 0;

			const auto base = stats.GetStat( base_number_of_projectiles_in_spiral_nova ) ? stats.GetStat( base_number_of_projectiles_in_spiral_nova ) : 1;
			return stats.GetStat( number_of_additional_arrows ) + base;
		}

		VIRTUAL_STAT( number_of_zombies_allowed,
			base_number_of_zombies_allowed,
			number_of_zombies_allowed_pluspercent,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return std::max( stats.GetStat( base_number_of_zombies_allowed ) * ( 100 + stats.GetStat( number_of_zombies_allowed_pluspercent ) ) / 100, 0 );
		}

		VIRTUAL_STAT( number_of_spectres_allowed,
			base_number_of_spectres_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( base_number_of_spectres_allowed );
		}

		VIRTUAL_STAT( number_of_converted_allowed,
			base_number_of_converted_allowed )
		{
			return stats.GetStat( base_number_of_converted_allowed );
		}

		VIRTUAL_STAT( number_of_skeletons_allowed,
			base_number_of_skeletons_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( base_number_of_skeletons_allowed );
		}

		VIRTUAL_STAT( number_of_raging_spirits_allowed,
			base_number_of_raging_spirits_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( base_number_of_raging_spirits_allowed );
		}

		VIRTUAL_STAT( number_of_essence_spirits_allowed,
			base_number_of_essence_spirits_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( base_number_of_essence_spirits_allowed );
		}

		VIRTUAL_STAT( number_of_golems_allowed,
			base_number_of_golems_allowed )
		{
			return stats.GetStat( base_number_of_golems_allowed );
		}

		VIRTUAL_STAT( virtual_number_of_inca_minions_allowed,
			number_of_inca_minions_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( number_of_inca_minions_allowed );
		}

		VIRTUAL_STAT( virtual_number_of_insects_allowed,
			number_of_insects_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( number_of_insects_allowed );
		}

		VIRTUAL_STAT( virtual_number_of_taniwha_tails_allowed,
			number_of_taniwha_tails_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( number_of_taniwha_tails_allowed );
		}

		VIRTUAL_STAT( virtual_number_of_wolves_allowed,
			number_of_wolves_allowed,
			you_cannot_have_non_golem_minions )
		{
			if( stats.GetStat( you_cannot_have_non_golem_minions ) )
				return 0;

			return stats.GetStat( number_of_wolves_allowed );
		}		

		VIRTUAL_STAT( mana_cost_pluspercent,
			base_mana_cost_minuspercent,
			mana_cost_pluspercent_when_on_low_life, on_low_life,
			no_mana_cost,
			mana_cost_pluspercent_while_not_low_mana, on_low_mana,
			mana_cost_pluspercent_on_totemified_aura_skills, skill_is_totemified, skill_is_aura_skill,
			mana_cost_minuspercent_per_endurance_charge, current_endurance_charges,
			mana_cost_pluspercent_on_consecrated_ground, on_consecrated_ground,
			movement_skills_cost_no_mana, skill_is_movement_skill,
			warcries_cost_no_mana, is_warcry,
			movement_skills_mana_cost_pluspercent,
			mana_cost_pluspercent_while_on_full_energy_shield, on_full_energy_shield )
		{
			if ( stats.GetStat( no_mana_cost ) ||
				( stats.GetStat( movement_skills_cost_no_mana ) && stats.GetStat( skill_is_movement_skill ) ) ||
				( stats.GetStat( warcries_cost_no_mana ) && stats.GetStat( is_warcry ) ) )
				return -100;

			return -stats.GetStat( base_mana_cost_minuspercent ) +
				( stats.GetStat( current_endurance_charges ) * -stats.GetStat( mana_cost_minuspercent_per_endurance_charge ) ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( mana_cost_pluspercent_when_on_low_life ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( mana_cost_pluspercent_on_consecrated_ground ) : 0 ) +
				( !stats.GetStat( on_low_mana ) ? stats.GetStat( mana_cost_pluspercent_while_not_low_mana ) : 0 ) +
				( stats.GetStat( skill_is_movement_skill ) ? stats.GetStat( movement_skills_mana_cost_pluspercent ) : 0 ) +
				( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( mana_cost_pluspercent_while_on_full_energy_shield ) : 0 ) +
				( ( stats.GetStat( skill_is_totemified ) && stats.GetStat( skill_is_aura_skill ) ) ? stats.GetStat( mana_cost_pluspercent_on_totemified_aura_skills ) : 0 );
		}

		VIRTUAL_STAT( chance_to_dodge_percent,
			acrobatics_additional_chance_to_dodge_percent,
			keystone_acrobatics,
			chance_to_dodge_percent_per_frenzy_charge, current_frenzy_charges,
			base_chance_to_dodge_percent,
			maximum_dodge_chance_percent,
			have_taken_spell_damage_recently, dodge_chance_pluspercent_if_you_have_taken_spell_damage_recently,
			chance_to_dodge_attacks_percent_while_phasing, virtual_phase_through_objects )
		{
			const auto base_dodge = ( stats.GetStat( keystone_acrobatics ) ? 30 : 0 ) +
				stats.GetStat( base_chance_to_dodge_percent ) +
				stats.GetStat( chance_to_dodge_percent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				( stats.GetStat( have_taken_spell_damage_recently ) ? stats.GetStat( dodge_chance_pluspercent_if_you_have_taken_spell_damage_recently ) : 0 ) +
				( stats.GetStat( virtual_phase_through_objects ) ? stats.GetStat( chance_to_dodge_attacks_percent_while_phasing ) : 0 );

			return ( base_dodge > 0 ? std::min( base_dodge + stats.GetStat( acrobatics_additional_chance_to_dodge_percent ), stats.GetStat( maximum_dodge_chance_percent ) ) : 0 );
		}

		VIRTUAL_STAT( chance_to_dodge_spells_percent,
			phase_acrobatics_additional_chance_to_dodge_spells_percent,
			keystone_phase_acrobatics,
			base_chance_to_dodge_spells_percent,
			maximum_spell_dodge_chance_percent,
			chance_to_dodge_spells_percent_while_phased, virtual_phase_through_objects,
			have_taken_attack_damage_recently, spell_dodge_chance_pluspercent_if_you_have_taken_attack_damage_recently )
		{
			const auto base_dodge = ( stats.GetStat( keystone_phase_acrobatics ) ? 30 : 0 ) +
				stats.GetStat( base_chance_to_dodge_spells_percent ) +
				( stats.GetStat( virtual_phase_through_objects ) ? stats.GetStat( chance_to_dodge_spells_percent_while_phased ) : 0 ) +
				( stats.GetStat( have_taken_attack_damage_recently ) ? stats.GetStat( spell_dodge_chance_pluspercent_if_you_have_taken_attack_damage_recently ) : 0 );

			return ( base_dodge > 0 ? std::min( base_dodge + stats.GetStat( phase_acrobatics_additional_chance_to_dodge_spells_percent ), stats.GetStat( maximum_spell_dodge_chance_percent ) ) : 0 );
		}

		VIRTUAL_STAT( chance_to_evade_projectile_attacks_pluspercent,
			keystone_projectile_evasion )
		{
			return ( stats.GetStat( keystone_projectile_evasion ) ? 40 : 0 );
		}

		VIRTUAL_STAT( chance_to_evade_melee_attacks_pluspercent,
			keystone_projectile_evasion )
		{
			return ( stats.GetStat( keystone_projectile_evasion ) ? -20 : 0 );
		}

		VIRTUAL_STAT( keystone_acrobatics_energy_shield_pluspercent_final,
			keystone_acrobatics )
		{
			return ( stats.GetStat( keystone_acrobatics ) ? -50 : 0 );
		}

		VIRTUAL_STAT( keystone_acrobatics_physical_damage_reduction_rating_pluspercent_final,
			keystone_acrobatics )
		{
			return ( stats.GetStat( keystone_acrobatics ) ? -50 : 0 );
		}

		VIRTUAL_STAT( use_life_in_place_of_mana,
			base_use_life_in_place_of_mana,
			keystone_blood_magic,
			skill_is_attack, attacks_use_life_in_place_of_mana )
		{
			return ( ( stats.GetStat( base_use_life_in_place_of_mana )
				|| stats.GetStat( keystone_blood_magic )
				|| ( stats.GetStat( skill_is_attack ) && stats.GetStat( attacks_use_life_in_place_of_mana ) ) ) ? 1 : 0 );
		}

		VIRTUAL_STAT( no_mana,
			keystone_blood_magic )
		{
			return ( stats.GetStat( keystone_blood_magic ) ? 1 : 0 );
		}

		VIRTUAL_STAT( pain_attunement_keystone_spell_damage_pluspercent_final,
			keystone_pain_attunement, on_low_life )
		{
			if ( !stats.GetStat( keystone_pain_attunement ) )
				return 0;

			return ( stats.GetStat( on_low_life ) ? 30 : 0 );
		}

		VIRTUAL_STAT( item_found_rarity_pluspercent,
			base_item_found_rarity_pluspercent,
			item_found_rarity_pluspercent_when_on_low_life, on_low_life,
			cannot_increase_rarity_of_dropped_items,
			item_rarity_pluspercent_while_using_flask, using_flask,
			item_found_rarity_pluspercent_while_phasing, virtual_phase_through_objects,
			item_found_rarity_pluspercent_if_wearing_a_normal_item, number_of_equipped_normal_items )
		{
			if ( stats.GetStat( cannot_increase_rarity_of_dropped_items ) )
				return 0;

			return stats.GetStat( base_item_found_rarity_pluspercent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( item_found_rarity_pluspercent_when_on_low_life ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( item_rarity_pluspercent_while_using_flask ) : 0 ) +
				( stats.GetStat( virtual_phase_through_objects ) ? stats.GetStat( item_found_rarity_pluspercent_while_phasing ) : 0 ) +
				( stats.GetStat( number_of_equipped_normal_items ) ? stats.GetStat( item_found_rarity_pluspercent_if_wearing_a_normal_item ) : 0 );
		}

		VIRTUAL_STAT( item_found_quantity_pluspercent,
			base_item_found_quantity_pluspercent,
			item_found_quantity_pluspercent_when_on_low_life, on_low_life,
			cannot_increase_quantity_of_dropped_items,
			item_found_quantity_pluspercent_if_wearing_a_magic_item, number_of_equipped_magic_items )
		{
			if ( stats.GetStat( cannot_increase_quantity_of_dropped_items ) )
				return 0;

			return stats.GetStat( base_item_found_quantity_pluspercent ) +
				( stats.GetStat( on_low_life ) ? stats.GetStat( item_found_quantity_pluspercent_when_on_low_life ) : 0 ) +
				( stats.GetStat( number_of_equipped_magic_items ) ? stats.GetStat( item_found_quantity_pluspercent_if_wearing_a_magic_item ) : 0 );
		}

		VIRTUAL_STAT( bonus_damage_pluspercent_from_strength,
			strength,
			melee_damage_bonus_attributes_from_jewels )
		{
			return Round( ( stats.GetStat( strength ) + stats.GetStat( melee_damage_bonus_attributes_from_jewels ) ) / 5.0f ); // 5 points strength = 1% physical attack damage increase (can be other increases if modified)
		}

		VIRTUAL_STAT( spell_damage_pluspercent_from_dexterity,
			dexterity,
			agile_will )
		{
			return stats.GetStat( agile_will ) ? Round( stats.GetStat( dexterity ) / 5.0f ) : 0; // 5 points dexterity = 1% spell damage increase
		}

		VIRTUAL_STAT( share_endurance_charges_with_party_within_distance,
			keystone_conduit,
			endurance_only_conduit )
		{
			return ( stats.GetStat( keystone_conduit ) || stats.GetStat( endurance_only_conduit ) ) ? 120 : 0;
		}

		VIRTUAL_STAT( share_frenzy_charges_with_party_within_distance,
			keystone_conduit,
			frenzy_only_conduit )
		{
			return ( stats.GetStat( keystone_conduit ) || stats.GetStat( frenzy_only_conduit ) ) ? 120 : 0;
		}

		VIRTUAL_STAT( share_power_charges_with_party_within_distance,
			keystone_conduit,
			power_only_conduit )
		{
			return ( stats.GetStat( keystone_conduit ) || stats.GetStat( power_only_conduit ) ) ? 120 : 0;
		}

		VIRTUAL_STAT( minions_explode_on_low_life_maximum_life_percent_to_deal,
			keystone_minion_instability )
		{
			return stats.GetStat( keystone_minion_instability ) ? 33 : 0;
		}

		VIRTUAL_STAT( monster_attack_cast_speed_pluspercent_and_damage_minuspercent_final,
			monster_base_type_attack_cast_speed_pluspercent_and_damage_minuspercent_final,
			monster_rarity_attack_cast_speed_pluspercent_and_damage_minuspercent_final )
		{
			return std::max( stats.GetStat( monster_base_type_attack_cast_speed_pluspercent_and_damage_minuspercent_final ), stats.GetStat( monster_rarity_attack_cast_speed_pluspercent_and_damage_minuspercent_final ) );
		}

		VIRTUAL_STAT( totem_duration,
			base_totem_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final,
			totem_duration_pluspercent )
		{
			return Round( ( ( stats.GetStat( base_totem_duration ) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) + stats.GetStat( totem_duration_pluspercent ) ) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( minion_duration
			, base_skill_effect_duration
			, virtual_additional_skill_effect_duration
			, virtual_skill_effect_duration_pluspercent
			, virtual_skill_effect_duration_pluspercent_final
			, skill_specific_minion_duration_pluspercent_final
			, base_minion_duration_pluspercent
			, minion_duration_pluspercent_per_active_zombie, number_of_active_zombies
			)
		{
			return Round( (
				( stats.GetStat( base_skill_effect_duration ) + stats.GetStat( virtual_additional_skill_effect_duration ) ) *
				Scale( 100 +
					stats.GetStat( virtual_skill_effect_duration_pluspercent ) +
					stats.GetStat( base_minion_duration_pluspercent ) +
					stats.GetStat( minion_duration_pluspercent_per_active_zombie ) * stats.GetStat( number_of_active_zombies )
					) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) * 
				Scale( 100 + stats.GetStat( skill_specific_minion_duration_pluspercent_final ) )
				);
		}

		VIRTUAL_STAT( trap_duration,
			base_trap_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final,
			trap_duration_pluspercent )
		{
			return Round( (
				( stats.GetStat( base_trap_duration ) ) *
				Scale( 100 +
					stats.GetStat( virtual_skill_effect_duration_pluspercent ) +
					stats.GetStat( trap_duration_pluspercent ) ) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( mine_duration,
			base_mine_duration,
			virtual_skill_effect_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent_final,
			mine_duration_pluspercent )
		{
			return Round( ( ( stats.GetStat( base_mine_duration ) ) *
				Scale( 100 +
					stats.GetStat( virtual_skill_effect_duration_pluspercent ) +
					stats.GetStat( mine_duration_pluspercent ) ) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( totem_range,
			base_totem_range,
			totem_range_pluspercent )
		{
			return Round( stats.GetStat( base_totem_range ) * Scale( 100 + stats.GetStat( totem_range_pluspercent ) ) );
		}

		VIRTUAL_STAT( totem_level,
			base_active_skill_totem_level,
			totem_support_gem_level )
		{
			if ( stats.GetStat( base_active_skill_totem_level ) )
				return stats.GetStat( base_active_skill_totem_level );
			else
				return stats.GetStat( totem_support_gem_level );
		}

		VIRTUAL_STAT( number_of_totems_allowed,
			base_number_of_totems_allowed,
			number_of_additional_totems_allowed,
			active_skill_index, number_of_additional_siege_ballistae_per_200_dexterity, dexterity,
			attack_skills_additional_totems_allowed, skill_is_attack )
		{
			if ( !stats.GetStat( base_number_of_totems_allowed ) )
				return 0;

			return stats.GetStat( base_number_of_totems_allowed ) +
				stats.GetStat( number_of_additional_totems_allowed ) +
				( stats.GetStat( skill_is_attack ) ? stats.GetStat( attack_skills_additional_totems_allowed ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::siege_ballista ) ? ( stats.GetStat( number_of_additional_siege_ballistae_per_200_dexterity ) * stats.GetStat( dexterity ) / 200 ) : 0 );
		}

		VIRTUAL_STAT( skill_display_number_of_totems_allowed,
			number_of_totems_allowed, is_totem )
		{
			if ( !stats.GetStat( is_totem ) )
				return 0;

			return stats.GetStat( number_of_totems_allowed );
		}

		VIRTUAL_STAT( number_of_traps_allowed,
			base_number_of_traps_allowed,
			number_of_additional_traps_allowed )
		{
			if ( !stats.GetStat( base_number_of_traps_allowed ) )
				return 0;

			return stats.GetStat( base_number_of_traps_allowed ) + stats.GetStat( number_of_additional_traps_allowed );
		}

		VIRTUAL_STAT( skill_display_number_of_traps_allowed,
			number_of_traps_allowed, is_trap )
		{
			if ( !stats.GetStat( is_trap ) )
				return 0;

			return stats.GetStat( number_of_traps_allowed );
		}

		VIRTUAL_STAT( number_of_remote_mines_allowed,
			base_number_of_remote_mines_allowed,
			number_of_additional_remote_mines_allowed )
		{
			if ( !stats.GetStat( base_number_of_remote_mines_allowed ) )
				return 0;

			return stats.GetStat( base_number_of_remote_mines_allowed ) + stats.GetStat( number_of_additional_remote_mines_allowed );
		}

		VIRTUAL_STAT( skill_display_number_of_remote_mines_allowed,
			number_of_remote_mines_allowed, is_remote_mine )
		{
			if ( !stats.GetStat( is_remote_mine ) )
				return 0;

			return stats.GetStat( number_of_remote_mines_allowed );
		}

		VIRTUAL_STAT( monster_ground_fire_on_death_duration_ms,
			monster_ground_effect_on_death_base_duration_ms,
			virtual_skill_effect_duration_pluspercent,
			support_reduced_duration_skill_effect_duration_pluspercent_final,
			virtual_ignite_duration_pluspercent )
		{
			return Round( (stats.GetStat( monster_ground_effect_on_death_base_duration_ms ) * Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) + stats.GetStat( virtual_ignite_duration_pluspercent ) ))* Scale(
				100 + stats.GetStat( support_reduced_duration_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( monster_caustic_cloud_on_death_duration_ms,
			monster_ground_effect_on_death_base_duration_ms,
			virtual_skill_effect_duration_pluspercent,
			support_reduced_duration_skill_effect_duration_pluspercent_final )
		{
			return Round( (stats.GetStat( monster_ground_effect_on_death_base_duration_ms ) * Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) ) )* Scale(
				100 + stats.GetStat( support_reduced_duration_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( monster_ground_ice_on_death_duration_ms,
			monster_ground_effect_on_death_base_duration_ms,
			virtual_skill_effect_duration_pluspercent,
			support_reduced_duration_skill_effect_duration_pluspercent_final,
			virtual_chill_duration_pluspercent )
		{
			return Round( (stats.GetStat( monster_ground_effect_on_death_base_duration_ms ) * Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) + stats.GetStat( virtual_chill_duration_pluspercent ) ))* Scale(
				100 + stats.GetStat( support_reduced_duration_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( monster_ground_tar_on_death_duration_ms,
			monster_ground_effect_on_death_base_duration_ms,
			virtual_skill_effect_duration_pluspercent,
			support_reduced_duration_skill_effect_duration_pluspercent_final )
		{
			return Round( (stats.GetStat( monster_ground_effect_on_death_base_duration_ms ) * Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent ) ))* Scale(
				100 + stats.GetStat( support_reduced_duration_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( main_hand_chance_to_blind_on_hit_percent,
			main_hand_local_chance_to_blind_on_hit_percent,
			global_chance_to_blind_on_hit_percent )
		{
			return stats.GetStat( main_hand_local_chance_to_blind_on_hit_percent ) + stats.GetStat( global_chance_to_blind_on_hit_percent );
		}

		VIRTUAL_STAT( off_hand_chance_to_blind_on_hit_percent,
			off_hand_local_chance_to_blind_on_hit_percent,
			global_chance_to_blind_on_hit_percent )
		{
			return stats.GetStat( off_hand_local_chance_to_blind_on_hit_percent ) + stats.GetStat( global_chance_to_blind_on_hit_percent );
		}

		VIRTUAL_STAT( display_minion_base_maximum_life,
			display_minion_monster_type,
			display_minion_monster_level )
		{
			//prevent anything that doesn't need this stat doing the lookup
			if ( stats.GetStat( display_minion_monster_type ) == 0 || stats.GetStat( display_minion_monster_level ) == 0 )
				return 0;

			const Loaders::DisplayMinionMonsterTypeHandle minion_type_table( L"Data/DisplayMinionMonsterType.dat" );
			//access minion type table by decimal key, NOT by index
			const auto minion_type = minion_type_table->GetDataRowByKey( minion_type_table, stats.GetStat( display_minion_monster_type ) );
			if( minion_type.IsNull() ) //unknown monster type
				return 0;

			//look up the default monster stats for that level of monster
			const Loaders::DefaultMonsterStatsHandle default_stat_table( L"Data/DefaultMonsterStats.dat" );
			const auto monster_level_stats = Loaders::DefaultMonsterStats::GetDataRowByIndex( default_stat_table, stats.GetStat( display_minion_monster_level ) - 1 );

			const bool summoned = minion_type->GetMonsterVariety()->Getmonster_type( )->GetIsPlayerSummoned( );
			const int life = summoned ? monster_level_stats->GetSummonedLife() : monster_level_stats->GetMonsterLife();
			return life * minion_type->GetMonsterVariety()->GetRelativeDefensiveness( ) / 100;
		}

		VIRTUAL_STAT( display_minion_maximum_life,
			display_minion_base_maximum_life,
			display_minion_monster_type,
			zombie_maximum_life_pluspercent,
			skeleton_maximum_life_pluspercent,
			active_skill_minion_life_pluspercent_final,
			minion_maximum_life_pluspercent,
			zombie_maximum_life_plus )
		{
			if ( stats.GetStat( display_minion_monster_type ) == 0 )
				return 0;

			//golems, animated guardian and clones have no special life stats
			auto life_pluspercent = stats.GetStat( minion_maximum_life_pluspercent );
			auto life_plus = 0;

			if ( stats.GetStat( display_minion_monster_type ) == 2 ) //1 is zombies, 2 is skeletons
			{
				life_pluspercent = stats.GetStat( skeleton_maximum_life_pluspercent );
				life_plus = 0;
			}
			else if ( stats.GetStat( display_minion_monster_type ) == 1 )
			{
				life_pluspercent = stats.GetStat( zombie_maximum_life_pluspercent );
				life_plus = stats.GetStat( zombie_maximum_life_plus );
			}

			return Round( ( stats.GetStat( display_minion_base_maximum_life ) + life_plus ) * Scale( 100 + life_pluspercent ) * Scale( 100 + stats.GetStat( active_skill_minion_life_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( physical_damage_percent_to_convert_to_fire,
			base_physical_damage_percent_to_convert_to_fire,
			base_physical_damage_percent_to_convert_to_cold,
			base_physical_damage_percent_to_convert_to_lightning,
			base_physical_damage_percent_to_convert_to_chaos,
			skill_physical_damage_percent_to_convert_to_fire,
			skill_physical_damage_percent_to_convert_to_cold,
			skill_physical_damage_percent_to_convert_to_lightning,
			skill_physical_damage_percent_to_convert_to_chaos,
			convert_all_physical_damage_to_fire )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			const int converted_to_fire = Round( stats.GetStat( skill_physical_damage_percent_to_convert_to_fire ) + base_scale * stats.GetStat( base_physical_damage_percent_to_convert_to_fire ) );

			const int total_converted = total_skill_converted + total_base_converted;

			//if the convert all to fire stat is set, also convert any unconverted damage
			if ( total_converted < 100 && stats.GetStat( convert_all_physical_damage_to_fire ) )
				return ( 100 - total_converted ) + converted_to_fire;
			else
			{
				return converted_to_fire;
			}
		}

		VIRTUAL_STAT( physical_damage_percent_to_convert_to_cold,
			base_physical_damage_percent_to_convert_to_fire,
			base_physical_damage_percent_to_convert_to_cold,
			base_physical_damage_percent_to_convert_to_lightning,
			base_physical_damage_percent_to_convert_to_chaos,
			skill_physical_damage_percent_to_convert_to_fire,
			skill_physical_damage_percent_to_convert_to_cold,
			skill_physical_damage_percent_to_convert_to_lightning,
			skill_physical_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_physical_damage_percent_to_convert_to_cold ) + base_scale * stats.GetStat( base_physical_damage_percent_to_convert_to_cold ) );
		}

		VIRTUAL_STAT( physical_damage_percent_to_convert_to_lightning,
			base_physical_damage_percent_to_convert_to_fire,
			base_physical_damage_percent_to_convert_to_cold,
			base_physical_damage_percent_to_convert_to_lightning,
			base_physical_damage_percent_to_convert_to_chaos,
			skill_physical_damage_percent_to_convert_to_fire,
			skill_physical_damage_percent_to_convert_to_cold,
			skill_physical_damage_percent_to_convert_to_lightning,
			skill_physical_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_physical_damage_percent_to_convert_to_lightning ) + base_scale * stats.GetStat( base_physical_damage_percent_to_convert_to_lightning ) );
		}

		VIRTUAL_STAT( physical_damage_percent_to_convert_to_chaos,
			base_physical_damage_percent_to_convert_to_fire,
			base_physical_damage_percent_to_convert_to_cold,
			base_physical_damage_percent_to_convert_to_lightning,
			base_physical_damage_percent_to_convert_to_chaos,
			skill_physical_damage_percent_to_convert_to_fire,
			skill_physical_damage_percent_to_convert_to_cold,
			skill_physical_damage_percent_to_convert_to_lightning,
			skill_physical_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( skill_physical_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_physical_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_cold ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_lightning ) +
				stats.GetStat( base_physical_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_physical_damage_percent_to_convert_to_chaos ) + base_scale * stats.GetStat( base_physical_damage_percent_to_convert_to_chaos ) );
		}

		VIRTUAL_STAT( lightning_damage_percent_to_convert_to_cold,
			base_lightning_damage_percent_to_convert_to_fire,
			base_lightning_damage_percent_to_convert_to_cold,
			base_lightning_damage_percent_to_convert_to_chaos,
			skill_lightning_damage_percent_to_convert_to_fire,
			skill_lightning_damage_percent_to_convert_to_cold,
			skill_lightning_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_lightning_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_lightning_damage_percent_to_convert_to_cold ) +
				stats.GetStat( base_lightning_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_lightning_damage_percent_to_convert_to_cold ) + base_scale * stats.GetStat( base_lightning_damage_percent_to_convert_to_cold ) );
		}

		VIRTUAL_STAT( lightning_damage_percent_to_convert_to_fire,
			base_lightning_damage_percent_to_convert_to_fire,
			base_lightning_damage_percent_to_convert_to_cold,
			base_lightning_damage_percent_to_convert_to_chaos,
			skill_lightning_damage_percent_to_convert_to_fire,
			skill_lightning_damage_percent_to_convert_to_cold,
			skill_lightning_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_lightning_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_lightning_damage_percent_to_convert_to_cold ) +
				stats.GetStat( base_lightning_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_lightning_damage_percent_to_convert_to_fire ) + base_scale * stats.GetStat( base_lightning_damage_percent_to_convert_to_fire ) );
		}

		VIRTUAL_STAT( lightning_damage_percent_to_convert_to_chaos
			, base_lightning_damage_percent_to_convert_to_fire
			, base_lightning_damage_percent_to_convert_to_cold
			, base_lightning_damage_percent_to_convert_to_chaos
			, base_lightning_damage_percent_to_convert_to_chaos_60percent_value
			, skill_lightning_damage_percent_to_convert_to_fire
			, skill_lightning_damage_percent_to_convert_to_cold
			, skill_lightning_damage_percent_to_convert_to_chaos
			)
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_cold ) +
				stats.GetStat( skill_lightning_damage_percent_to_convert_to_chaos ) );

			const auto base_amount =
				stats.GetStat( base_lightning_damage_percent_to_convert_to_chaos ) +
				stats.GetStat( base_lightning_damage_percent_to_convert_to_chaos_60percent_value ) * 60 / 100;

			const auto total_base_converted =
				stats.GetStat( base_lightning_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_lightning_damage_percent_to_convert_to_cold ) +
				base_amount;

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_lightning_damage_percent_to_convert_to_chaos ) + base_scale * base_amount );
		}

		VIRTUAL_STAT( cold_damage_percent_to_convert_to_fire,
			base_cold_damage_percent_to_convert_to_fire,
			base_cold_damage_percent_to_convert_to_chaos,
			skill_cold_damage_percent_to_convert_to_fire,
			skill_cold_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_cold_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_cold_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_cold_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_cold_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_cold_damage_percent_to_convert_to_fire ) + base_scale * stats.GetStat( base_cold_damage_percent_to_convert_to_fire ) );
		}

		VIRTUAL_STAT( cold_damage_percent_to_convert_to_chaos,
			base_cold_damage_percent_to_convert_to_fire,
			base_cold_damage_percent_to_convert_to_chaos,
			skill_cold_damage_percent_to_convert_to_fire,
			skill_cold_damage_percent_to_convert_to_chaos )
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_cold_damage_percent_to_convert_to_fire ) +
				stats.GetStat( skill_cold_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_cold_damage_percent_to_convert_to_fire ) +
				stats.GetStat( base_cold_damage_percent_to_convert_to_chaos );

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_cold_damage_percent_to_convert_to_chaos ) + base_scale * stats.GetStat( base_cold_damage_percent_to_convert_to_chaos ) );
		}

		VIRTUAL_STAT( fire_damage_percent_to_convert_to_chaos
			, base_fire_damage_percent_to_convert_to_chaos
			, base_fire_damage_percent_to_convert_to_chaos_60percent_value
			, skill_fire_damage_percent_to_convert_to_chaos
			)
		{
			const auto total_skill_converted = std::min( 100,
				stats.GetStat( skill_fire_damage_percent_to_convert_to_chaos ) );

			const auto total_base_converted =
				stats.GetStat( base_fire_damage_percent_to_convert_to_chaos ) + 
				stats.GetStat( base_fire_damage_percent_to_convert_to_chaos_60percent_value ) * 60 / 100;

			const float base_scale = ( total_skill_converted + total_base_converted > 100 ) ? ( 100 - total_skill_converted ) / float( total_base_converted ) : 1.0f;

			return Round( stats.GetStat( skill_fire_damage_percent_to_convert_to_chaos ) + base_scale * total_base_converted );
		}

		VIRTUAL_STAT( total_physical_damage_percent_as_fire,
			physical_damage_percent_to_convert_to_fire,
			physical_damage_percent_to_add_as_fire,
			wand_physical_damage_percent_to_add_as_fire, 
			main_hand_weapon_type, 
			off_hand_weapon_type, 
			skill_can_fire_wand_projectiles,
			sword_physical_damage_percent_to_add_as_fire, casting_spell,
			attack_physical_damage_percent_to_add_as_fire, skill_is_attack,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const auto off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool is_sword = main_hand_all_1h_weapons_count || off_hand_all_1h_weapons_count || main_hand_weapon_index == Items::OneHandSword ||
				main_hand_weapon_index == Items::OneHandSwordThrusting ||
				main_hand_weapon_index == Items::TwoHandSword;

			return stats.GetStat( physical_damage_percent_to_convert_to_fire ) + stats.GetStat( physical_damage_percent_to_add_as_fire ) +
				( stats.GetStat( skill_can_fire_wand_projectiles ) && main_hand_weapon_index == Items::Wand ? stats.GetStat( wand_physical_damage_percent_to_add_as_fire ) : 0 ) +
				( ( !stats.GetStat( casting_spell ) && is_sword ) ? stats.GetStat( sword_physical_damage_percent_to_add_as_fire ) : 0 ) +
				( stats.GetStat( skill_is_attack ) ? stats.GetStat( attack_physical_damage_percent_to_add_as_fire ) : 0 );
		}

		VIRTUAL_STAT( total_physical_damage_percent_as_cold,
			physical_damage_percent_to_convert_to_cold,
			physical_damage_percent_to_add_as_cold,
			wand_physical_damage_percent_to_add_as_cold, main_hand_weapon_type, skill_can_fire_wand_projectiles )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( physical_damage_percent_to_convert_to_cold ) + stats.GetStat( physical_damage_percent_to_add_as_cold ) +
				( stats.GetStat( skill_can_fire_wand_projectiles ) && main_hand_weapon_index == Items::Wand ? stats.GetStat( wand_physical_damage_percent_to_add_as_cold ) : 0 );
		}

		VIRTUAL_STAT( total_physical_damage_percent_as_lightning,
			physical_damage_percent_to_convert_to_lightning,
			physical_damage_percent_to_add_as_lightning,
			wand_physical_damage_percent_to_add_as_lightning, main_hand_weapon_type, skill_can_fire_wand_projectiles,
			attack_physical_damage_percent_to_add_as_lightning, skill_is_attack )
		{
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			return stats.GetStat( physical_damage_percent_to_convert_to_lightning ) + stats.GetStat( physical_damage_percent_to_add_as_lightning ) +
				( stats.GetStat( skill_can_fire_wand_projectiles ) && main_hand_weapon_index == Items::Wand ? stats.GetStat( wand_physical_damage_percent_to_add_as_lightning ) : 0 ) +
				( ( stats.GetStat( skill_is_attack ) ? stats.GetStat( attack_physical_damage_percent_to_add_as_lightning ) : 0 ) );
		}

		VIRTUAL_STAT( total_physical_damage_percent_as_chaos,
			physical_damage_percent_to_convert_to_chaos,
			physical_damage_percent_to_add_as_chaos )
		{
			return stats.GetStat( physical_damage_percent_to_convert_to_chaos ) + stats.GetStat( physical_damage_percent_to_add_as_chaos );
		}

		VIRTUAL_STAT( total_fire_damage_percent_as_chaos,
			fire_damage_percent_to_convert_to_chaos,
			fire_damage_percent_to_add_as_chaos,
			elemental_damage_percent_to_add_as_chaos )
		{
			return stats.GetStat( fire_damage_percent_to_convert_to_chaos ) + stats.GetStat( fire_damage_percent_to_add_as_chaos ) + stats.GetStat( elemental_damage_percent_to_add_as_chaos );
		}

		VIRTUAL_STAT( total_cold_damage_percent_as_fire,
			cold_damage_percent_to_convert_to_fire,
			cold_damage_percent_to_add_as_fire )
		{
			return stats.GetStat( cold_damage_percent_to_convert_to_fire ) + stats.GetStat( cold_damage_percent_to_add_as_fire );
		}

		VIRTUAL_STAT( total_cold_damage_percent_as_chaos,
			cold_damage_percent_to_convert_to_chaos,
			cold_damage_percent_to_add_as_chaos,
			elemental_damage_percent_to_add_as_chaos )
		{
			return stats.GetStat( cold_damage_percent_to_convert_to_chaos ) + stats.GetStat( cold_damage_percent_to_add_as_chaos ) + stats.GetStat( elemental_damage_percent_to_add_as_chaos );
		}

		VIRTUAL_STAT( total_lightning_damage_percent_as_fire,
			lightning_damage_percent_to_convert_to_fire,
			lightning_damage_percent_to_add_as_fire )
		{
			return stats.GetStat( lightning_damage_percent_to_convert_to_fire ) + stats.GetStat( lightning_damage_percent_to_add_as_fire );
		}

		VIRTUAL_STAT( total_lightning_damage_percent_as_cold,
			lightning_damage_percent_to_convert_to_cold,
			lightning_damage_percent_to_add_as_cold )
		{
			return stats.GetStat( lightning_damage_percent_to_convert_to_cold ) + stats.GetStat( lightning_damage_percent_to_add_as_cold );
		}

		VIRTUAL_STAT( total_lightning_damage_percent_as_chaos,
			lightning_damage_percent_to_convert_to_chaos,
			lightning_damage_percent_to_add_as_chaos,
			elemental_damage_percent_to_add_as_chaos )
		{
			return stats.GetStat( lightning_damage_percent_to_convert_to_chaos ) + stats.GetStat( lightning_damage_percent_to_add_as_chaos ) + stats.GetStat( elemental_damage_percent_to_add_as_chaos );
		}

		VIRTUAL_STAT( spell_damage_taken_pluspercent, spell_damage_taken_pluspercent_when_on_low_mana, on_low_mana )
		{
			if ( !!stats.GetStat( on_low_mana ) )
				return stats.GetStat( spell_damage_taken_pluspercent_when_on_low_mana );

			return 0;
		}

		VIRTUAL_STAT( damage_taken_pluspercent,
			base_damage_taken_pluspercent,
			damage_taken_pluspercent_per_frenzy_charge, current_frenzy_charges,
			damage_taken_pluspercent_per_bloodline_damage_charge, current_bloodline_damage_charges,
			clone_shot_damage_taken_pluspercent_final,
			damage_taken_pluspercent_while_es_full, on_full_energy_shield,
			have_been_savage_hit_recently, damage_taken_pluspercent_if_you_have_taken_a_savage_hit_recently,
			damage_taken_pluspercent_if_not_hit_recently_final, have_been_hit_in_past_4_seconds,
			damage_taken_pluspercent_if_taunted_an_enemy_recently, have_taunted_an_enemy_recently )
		{
			return Round( ( 100 +
				stats.GetStat( base_damage_taken_pluspercent ) +
				( stats.GetStat( on_full_energy_shield ) ? stats.GetStat( damage_taken_pluspercent_while_es_full ) : 0 ) +
				stats.GetStat( damage_taken_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				stats.GetStat( damage_taken_pluspercent_per_bloodline_damage_charge ) * stats.GetStat( current_bloodline_damage_charges ) +
				stats.GetStat( clone_shot_damage_taken_pluspercent_final ) +
				( stats.GetStat( have_taunted_an_enemy_recently ) ? stats.GetStat( damage_taken_pluspercent_if_taunted_an_enemy_recently ) : 0 ) +
					( stats.GetStat( have_been_savage_hit_recently ) ? stats.GetStat( damage_taken_pluspercent_if_you_have_taken_a_savage_hit_recently ) : 0 ) ) *
					Scale( 100 + ( stats.GetStat( have_been_hit_in_past_4_seconds ) ? 0 : stats.GetStat( damage_taken_pluspercent_if_not_hit_recently_final ) ) )
					- 100 );
		}

		VIRTUAL_STAT( physical_damage_percent_lost_to_conversion,
			physical_damage_percent_to_convert_to_fire,
			physical_damage_percent_to_convert_to_cold,
			physical_damage_percent_to_convert_to_lightning,
			physical_damage_percent_to_convert_to_chaos )
		{
			const auto total_converted_away = stats.GetStat( physical_damage_percent_to_convert_to_fire )
				+ stats.GetStat( physical_damage_percent_to_convert_to_cold )
				+ stats.GetStat( physical_damage_percent_to_convert_to_lightning )
				+ stats.GetStat( physical_damage_percent_to_convert_to_chaos );

			return std::min( 100, total_converted_away );
		}

		VIRTUAL_STAT( fire_damage_percent_lost_to_conversion,
			fire_damage_percent_to_convert_to_chaos )
		{
			const auto total_converted_away = stats.GetStat( fire_damage_percent_to_convert_to_chaos );

			return std::min( 100, total_converted_away );
		}

		VIRTUAL_STAT( cold_damage_percent_lost_to_conversion,
			cold_damage_percent_to_convert_to_fire,
			cold_damage_percent_to_convert_to_chaos )
		{
			const auto total_converted_away = stats.GetStat( cold_damage_percent_to_convert_to_fire ) + stats.GetStat( cold_damage_percent_to_convert_to_chaos );

			return std::min( 100, total_converted_away );
		}

		VIRTUAL_STAT( lightning_damage_percent_lost_to_conversion,
			lightning_damage_percent_to_convert_to_fire,
			lightning_damage_percent_to_convert_to_cold,
			lightning_damage_percent_to_convert_to_chaos )
		{
			const auto total_converted_away = stats.GetStat( lightning_damage_percent_to_convert_to_fire )
				+ stats.GetStat( lightning_damage_percent_to_convert_to_cold )
				+ stats.GetStat( lightning_damage_percent_to_convert_to_chaos );

			return std::min( 100, total_converted_away );
		}

		VIRTUAL_STAT( chaos_damage_percent_lost_to_conversion,
			deal_no_damage ) //NOTE: this is here because a virtual stat can't (currently) have zero dependencies, but we don't want to make this stat non-virtual either
		{
			//damage cannot be converted away from chaos
			return 0;
		}

		VIRTUAL_STAT( bleed_duration_pluspercent
			, bleed_duration_per_12_intelligence_pluspercent, intelligence
			)
		{
			//TODO: should generic duration modifiers be included? they traditionally are not, but that was before we had a bleed-specific duraiton modifier.
			return stats.GetStat( bleed_duration_per_12_intelligence_pluspercent ) * stats.GetStat( intelligence ) / 12;
		}

		//this duration is only used for puncture.
		VIRTUAL_STAT( bleed_on_hit_duration
			, bleed_on_hit_base_duration
			, virtual_skill_effect_duration_pluspercent
			, virtual_skill_effect_duration_pluspercent_final
			, buff_duration_pluspercent
			, buff_effect_duration_pluspercent_per_endurance_charge, current_endurance_charges
			, bleed_duration_pluspercent
			)
		{
			const auto base_bleed_duration = stats.GetStat( bleed_on_hit_base_duration );

			// No bleeding if bleed_on_hit_base_duration has a value of zero.
			return base_bleed_duration > 0 ?
				Round( ( base_bleed_duration ) *
					Scale( 100 +
						stats.GetStat( virtual_skill_effect_duration_pluspercent ) +
						stats.GetStat( buff_duration_pluspercent ) +
						stats.GetStat( bleed_duration_pluspercent ) +
						stats.GetStat( buff_effect_duration_pluspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) *
					Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) ) :
				0;
		}

		VIRTUAL_STAT( cannot_be_stunned,
			base_cannot_be_stunned,
			cannot_be_stunned_when_on_low_life, on_low_life,
			occultist_immune_to_stun_while_on_full_energy_shield, on_full_energy_shield,
			keystone_unwavering_stance,
			cannot_be_stunned_while_at_max_endurance_charges, current_endurance_charges, virtual_maximum_endurance_charges,
			cannot_be_stunned_while_leeching, is_leeching )
		{
			return stats.GetStat( base_cannot_be_stunned ) ||
				stats.GetStat( cannot_be_stunned_when_on_low_life ) && stats.GetStat( on_low_life ) ||
				stats.GetStat( occultist_immune_to_stun_while_on_full_energy_shield ) && stats.GetStat( on_full_energy_shield ) ||
				stats.GetStat( keystone_unwavering_stance ) || 
				( stats.GetStat( cannot_be_stunned_while_at_max_endurance_charges  ) && stats.GetStat( current_endurance_charges ) == stats.GetStat( virtual_maximum_endurance_charges ) ) ||
				( stats.GetStat( cannot_be_stunned_while_leeching ) && stats.GetStat( is_leeching ) );
		}

		VIRTUAL_STAT( display_disable_melee_weapons,
			disable_skill_if_melee_attack,
			casting_spell )
		{
			return stats.GetStat( disable_skill_if_melee_attack ) && !stats.GetStat( casting_spell );
		}

		VIRTUAL_STAT( main_hand_maximum_attack_distance,
			main_hand_base_maximum_attack_distance,
			main_hand_weapon_range_plus,
			melee_range_plus, main_hand_weapon_type,
			melee_range_plus_while_unarmed,
			melee_weapon_range_plus )
		{
			return stats.GetStat( main_hand_base_maximum_attack_distance ) +
				( Items::IsMelee[stats.GetStat( main_hand_weapon_type )] ? stats.GetStat( melee_range_plus ) : 0 ) +
				( Items::IsWeapon[stats.GetStat( main_hand_weapon_type )] ? stats.GetStat( main_hand_weapon_range_plus ) : 0 ) +
				( ( Items::IsMelee[stats.GetStat( main_hand_weapon_type )] && Items::IsWeapon[stats.GetStat( main_hand_weapon_type )] ) ? stats.GetStat( melee_weapon_range_plus ) : 0 ) +
				( stats.GetStat( main_hand_weapon_type ) == Items::Unarmed ? stats.GetStat( melee_range_plus_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( off_hand_maximum_attack_distance,
			off_hand_base_maximum_attack_distance,
			off_hand_weapon_range_plus,
			melee_range_plus, off_hand_weapon_type,
			melee_range_plus_while_unarmed,
			melee_weapon_range_plus )
		{
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			return stats.GetStat( off_hand_base_maximum_attack_distance ) +
				( Items::IsMelee[stats.GetStat( off_hand_weapon_type )] ? stats.GetStat( melee_range_plus ) : 0 ) +
				( Items::IsWeapon[stats.GetStat( off_hand_weapon_type )] ? stats.GetStat( off_hand_weapon_range_plus ) : 0 ) +
				( ( Items::IsMelee[stats.GetStat( off_hand_weapon_type )] && Items::IsWeapon[stats.GetStat( off_hand_weapon_type )] ) ? stats.GetStat( melee_weapon_range_plus ) : 0 ) +
				( off_hand_weapon_index == Items::Unarmed ? stats.GetStat( melee_range_plus_while_unarmed ) : 0 );
		}

		VIRTUAL_STAT( life_leech_is_instant,
			base_life_leech_is_instant,
			keystone_vampirism )
		{
			return stats.GetStat( base_life_leech_is_instant ) || stats.GetStat( keystone_vampirism );
		}

		VIRTUAL_STAT( leech_is_instant_on_critical,
			base_leech_is_instant_on_critical,
			unqiue_atzitis_acuity_instant_leech_60percent_effectiveness_on_crit )
		{
			return stats.GetStat( base_leech_is_instant_on_critical ) || stats.GetStat( unqiue_atzitis_acuity_instant_leech_60percent_effectiveness_on_crit );
		}

		VIRTUAL_STAT( life_gain_per_target,
			base_life_gain_per_target,
			active_skill_gem_added_damage_effectiveness_pluspercent_final,
			cannot_recover_life,
			life_and_mana_gain_per_hit )
		{
			if ( stats.GetStat( cannot_recover_life ) )
				return 0;

			return ( Round( ( stats.GetStat( base_life_gain_per_target ) + stats.GetStat( life_and_mana_gain_per_hit ) ) *
				Scale( 100 + stats.GetStat( active_skill_gem_added_damage_effectiveness_pluspercent_final ) ) ) );
		}

		VIRTUAL_STAT( virtual_mana_gain_per_target,
			mana_gain_per_target,
			life_and_mana_gain_per_hit )
		{
			return stats.GetStat( mana_gain_per_target ) +
				( stats.GetStat( life_and_mana_gain_per_hit ) );
		}

		VIRTUAL_STAT( virtual_energy_shield_gain_per_target,
			energy_shield_gain_per_target )
		{
			return stats.GetStat( energy_shield_gain_per_target );
		}

		VIRTUAL_STAT( main_hand_local_life_gain_per_target,
			life_gain_per_target,
			base_main_hand_local_life_gain_per_target,
			base_main_hand_local_life_and_mana_gain_per_target )
		{
			return stats.GetStat( life_gain_per_target ) +
				stats.GetStat( base_main_hand_local_life_gain_per_target ) +
				stats.GetStat( base_main_hand_local_life_and_mana_gain_per_target );
		}

		VIRTUAL_STAT( off_hand_local_life_gain_per_target,
			life_gain_per_target,
			base_off_hand_local_life_gain_per_target,
			base_off_hand_local_life_and_mana_gain_per_target )
		{
			return stats.GetStat( life_gain_per_target ) +
				stats.GetStat( base_off_hand_local_life_gain_per_target ) +
				stats.GetStat( base_off_hand_local_life_and_mana_gain_per_target );
		}

		VIRTUAL_STAT( main_hand_local_mana_gain_per_target,
			virtual_mana_gain_per_target,
			base_main_hand_local_mana_gain_per_target,
			base_main_hand_local_life_and_mana_gain_per_target )
		{
			return stats.GetStat( virtual_mana_gain_per_target ) +
				stats.GetStat( base_main_hand_local_mana_gain_per_target ) +
				stats.GetStat( base_main_hand_local_life_and_mana_gain_per_target );
		}

		VIRTUAL_STAT( off_hand_local_mana_gain_per_target,
			virtual_mana_gain_per_target,
			base_off_hand_local_mana_gain_per_target,
			base_off_hand_local_life_and_mana_gain_per_target )
		{
			return stats.GetStat( virtual_mana_gain_per_target ) +
				stats.GetStat( base_off_hand_local_mana_gain_per_target ) +
				stats.GetStat( base_off_hand_local_life_and_mana_gain_per_target );
		}

		VIRTUAL_STAT( keystone_vampirism_life_leech_amount_pluspercent_final,
			keystone_vampirism )
		{
			return 0;
		}

		VIRTUAL_STAT( life_leech_amount_pluspercent,
			keystone_vampirism_life_leech_amount_pluspercent_final )
		{
			return stats.GetStat( keystone_vampirism_life_leech_amount_pluspercent_final );
		}

		VIRTUAL_STAT( leech_amount_pluspercent_final_on_crit,
			unqiue_atzitis_acuity_instant_leech_60percent_effectiveness_on_crit )
		{
			return stats.GetStat( unqiue_atzitis_acuity_instant_leech_60percent_effectiveness_on_crit ) ? 0 : 0; //atziri no longer does leech at 60% effectiveness
		}

		VIRTUAL_STAT( life_gained_on_spell_hit,
			base_life_gained_on_spell_hit,
			cannot_recover_life )
		{
			if ( stats.GetStat( cannot_recover_life ) )
				return 0;

			return stats.GetStat( base_life_gained_on_spell_hit );
		}

		VIRTUAL_STAT( attack_repeat_count,
			base_attack_repeat_count,
			skill_repeat_count,
			base_melee_attack_repeat_count, attack_is_melee, casting_spell )
		{
			const bool is_melee = stats.GetStat( attack_is_melee ) && !stats.GetStat( casting_spell );
			return stats.GetStat( base_attack_repeat_count ) + stats.GetStat( skill_repeat_count ) + ( is_melee ? stats.GetStat( base_melee_attack_repeat_count ) : 0 );
		}

		VIRTUAL_STAT( spell_repeat_count,
			base_spell_repeat_count,
			skill_repeat_count )
		{
			return stats.GetStat( base_spell_repeat_count ) + stats.GetStat( skill_repeat_count );
		}


		VIRTUAL_STAT( killed_monster_dropped_item_quantity_pluspercent,
			base_killed_monster_dropped_item_quantity_pluspercent,
			cannot_increase_quantity_of_dropped_items )
		{
			return stats.GetStat( cannot_increase_quantity_of_dropped_items ) ? 0 : stats.GetStat( base_killed_monster_dropped_item_quantity_pluspercent );
		}

		VIRTUAL_STAT( killed_monster_dropped_item_rarity_pluspercent,
			base_killed_monster_dropped_item_rarity_pluspercent,
			cannot_increase_rarity_of_dropped_items )
		{
			return stats.GetStat( cannot_increase_rarity_of_dropped_items ) ? 0 : stats.GetStat( base_killed_monster_dropped_item_rarity_pluspercent );
		}

		VIRTUAL_STAT( enemy_extra_damage_rolls
			, enemy_extra_damage_rolls_when_on_low_life, on_low_life )
		{
			return stats.GetStat( on_low_life ) ? stats.GetStat( enemy_extra_damage_rolls_when_on_low_life ) : 0;
		}

		VIRTUAL_STAT( cannot_leech, base_cannot_leech,
			cannot_leech_when_on_low_life, on_low_life )
		{
			return stats.GetStat( base_cannot_leech ) || ( stats.GetStat( cannot_leech_when_on_low_life ) && stats.GetStat( on_low_life ) );
		}

		VIRTUAL_STAT( cannot_leech_life,
			cannot_leech,
			base_cannot_leech_life,
			cannot_recover_life )
		{
			return stats.GetStat( cannot_leech ) || stats.GetStat( base_cannot_leech_life ) || stats.GetStat( cannot_recover_life );
		}

		VIRTUAL_STAT( cannot_leech_mana,
			cannot_leech,
			base_cannot_leech_mana,
			cannot_leech_or_regenerate_mana )
		{
			return stats.GetStat( cannot_leech ) || stats.GetStat( base_cannot_leech_mana ) || stats.GetStat( cannot_leech_or_regenerate_mana );
		}

		VIRTUAL_STAT( enemy_on_low_life_damage_taken_pluspercent,
			enemy_on_low_life_damage_taken_pluspercent_per_frenzy_charge,
			current_frenzy_charges )
		{
			return stats.GetStat( enemy_on_low_life_damage_taken_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges );
		}

		VIRTUAL_STAT( attack_speed_pluspercent_per_frenzy_charge,
			base_attack_speed_pluspercent_per_frenzy_charge,
			attack_and_cast_speed_pluspercent_per_frenzy_charge )
		{
			return stats.GetStat( base_attack_speed_pluspercent_per_frenzy_charge ) + stats.GetStat( attack_and_cast_speed_pluspercent_per_frenzy_charge );
		}

		VIRTUAL_STAT( cast_speed_pluspercent_per_frenzy_charge,
			base_cast_speed_pluspercent_per_frenzy_charge,
			attack_and_cast_speed_pluspercent_per_frenzy_charge )
		{
			return stats.GetStat( base_cast_speed_pluspercent_per_frenzy_charge ) + stats.GetStat( attack_and_cast_speed_pluspercent_per_frenzy_charge );
		}

		VIRTUAL_STAT( cannot_gain_endurance_charges,
			cannot_gain_endurance_charges_while_have_onslaught, virtual_has_onslaught )
		{
			return stats.GetStat( cannot_gain_endurance_charges_while_have_onslaught ) && stats.GetStat( virtual_has_onslaught );
		}

		VIRTUAL_STAT( chance_to_gain_random_curse_when_hit_percent,
			chance_to_gain_random_curse_when_hit_percent_per_10_levels, level )
		{
			return stats.GetStat( chance_to_gain_random_curse_when_hit_percent_per_10_levels ) * stats.GetStat( level ) / 10;
		}

		VIRTUAL_STAT( number_of_traps_to_throw,
			number_of_additional_traps_to_throw, is_trap )
		{
			if ( !stats.GetStat( is_trap ) )
				return 0;

			return 1 + stats.GetStat( number_of_additional_traps_to_throw );
		}

		VIRTUAL_STAT( number_of_mines_to_place,
			number_of_additional_mines_to_place, is_remote_mine )
		{
			if ( !stats.GetStat( is_remote_mine ) )
				return 0;

			return 1 + stats.GetStat( number_of_additional_mines_to_place );
		}

		VIRTUAL_STAT( damage_not_from_skill_user,
			base_damage_not_from_skill_user,
			skill_is_totemified,
			skill_is_trapped,
			skill_is_mined )
		{
			return stats.GetStat( base_damage_not_from_skill_user ) || stats.GetStat( skill_is_totemified ) || stats.GetStat( skill_is_trapped ) || stats.GetStat( skill_is_mined );
		}

		VIRTUAL_STAT( virtual_frenzy_charge_duration_pluspercent,
			base_frenzy_charge_duration_pluspercent,
			frenzy_charge_duration_pluspercent_per_frenzy_charge, current_frenzy_charges,
			charge_duration_pluspercent )
		{
			return stats.GetStat( base_frenzy_charge_duration_pluspercent ) +
				( stats.GetStat( frenzy_charge_duration_pluspercent_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) ) +
				stats.GetStat( charge_duration_pluspercent );
		}

		VIRTUAL_STAT( attacks_poison,
			base_attacks_poison,
			attacks_poison_while_at_max_frenzy_charges, current_frenzy_charges, virtual_maximum_frenzy_charges )
		{
			return stats.GetStat( base_attacks_poison ) ||
				( stats.GetStat( attacks_poison_while_at_max_frenzy_charges ) ? stats.GetStat( current_frenzy_charges ) == stats.GetStat( virtual_maximum_frenzy_charges ) : false );
		}

		VIRTUAL_STAT( chance_to_ignite_percent,
			base_chance_to_ignite_percent,
			chance_to_freeze_shock_ignite_percent,
			projectile_ignite_chance_percent, is_projectile,
			enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			chance_to_ignite_percent_while_ignited, is_ignited,
			chance_to_ignite_percent_while_using_flask, using_flask,
			chance_to_freeze_shock_ignite_percent_during_flask_effect )
		{
			return stats.GetStat( base_chance_to_ignite_percent ) +
				( stats.GetStat( have_crit_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds ) ) +
				stats.GetStat( chance_to_freeze_shock_ignite_percent ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_ignite_chance_percent ) : 0 ) +
				( stats.GetStat( is_ignited ) ? stats.GetStat( chance_to_ignite_percent_while_ignited ) : 0 ) +
				( stats.GetStat( using_flask ) ?
					stats.GetStat( chance_to_ignite_percent_while_using_flask ) +
					stats.GetStat( chance_to_freeze_shock_ignite_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( chance_to_shock_percent,
			base_chance_to_shock_percent,
			chance_to_freeze_shock_ignite_percent,
			enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			projectile_shock_chance_percent, is_projectile,
			chance_to_shock_percent_while_using_flask, using_flask,
			chance_to_freeze_shock_ignite_percent_during_flask_effect )
		{
			return stats.GetStat( base_chance_to_shock_percent ) +
				( stats.GetStat( have_crit_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds ) ) +
				stats.GetStat( chance_to_freeze_shock_ignite_percent ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_shock_chance_percent ) : 0 ) +
				( stats.GetStat( using_flask ) ?
					stats.GetStat( chance_to_shock_percent_while_using_flask ) +
					stats.GetStat( chance_to_freeze_shock_ignite_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( chance_to_freeze_percent,
			base_chance_to_freeze_percent,
			chance_to_freeze_shock_ignite_percent,
			enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds, have_crit_in_past_4_seconds,
			projectile_freeze_chance_percent, is_projectile,
			chance_to_freeze_percent_while_using_flask, using_flask,
			chance_to_freeze_shock_ignite_percent_during_flask_effect )
		{
			return stats.GetStat( base_chance_to_freeze_percent ) +
				( stats.GetStat( have_crit_in_past_4_seconds ) ? 0 : stats.GetStat( enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds ) ) +
				stats.GetStat( chance_to_freeze_shock_ignite_percent ) +
				( stats.GetStat( is_projectile ) ? stats.GetStat( projectile_freeze_chance_percent ) : 0 ) +
				( stats.GetStat( using_flask ) ? 
					stats.GetStat( chance_to_freeze_percent_while_using_flask ) +
					stats.GetStat( chance_to_freeze_shock_ignite_percent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( damage_removed_from_mana_before_life_percent,
			base_damage_removed_from_mana_before_life_percent,
			current_power_charges, damage_taken_goes_to_mana_percent_per_power_charge,
			keystone_mana_shield )
		{
			return stats.GetStat( base_damage_removed_from_mana_before_life_percent ) +
				( stats.GetStat( keystone_mana_shield ) ? 30 : 0 ) +
				( stats.GetStat( damage_taken_goes_to_mana_percent_per_power_charge ) * stats.GetStat( current_power_charges ) );
		}

		VIRTUAL_STAT( actor_scale_pluspercent,
			base_actor_scale_pluspercent,
			capped_actor_scale_pluspercent )
		{
			return std::max( stats.GetStat( base_actor_scale_pluspercent ) +
				Clamp( stats.GetStat( capped_actor_scale_pluspercent ), -50, 50 ), -99 );
		}

		VIRTUAL_STAT( mana_reservation_pluspercent,
			base_mana_reservation_pluspercent,
			mortal_conviction_mana_reservation_pluspercent_final )
		{
			//multiplicative
			return Round( ( 100 
					+ stats.GetStat( base_mana_reservation_pluspercent ) )
				* Scale( 100 + stats.GetStat( mortal_conviction_mana_reservation_pluspercent_final ) )
				- 100 );
		}

		VIRTUAL_STAT( monster_will_be_deleted_on_death,
			base_monster_will_be_deleted_on_death,
			delete_on_death,
			corpse_cannot_be_destroyed )
		{
			return stats.GetStat( delete_on_death ) || ( stats.GetStat( base_monster_will_be_deleted_on_death ) && !stats.GetStat( corpse_cannot_be_destroyed ) );
		}

		VIRTUAL_STAT( life_leech_from_physical_damage_permyriad,
			base_life_leech_from_physical_damage_permyriad,
			old_do_not_use_base_life_leech_from_physical_damage_permyriad,
			life_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( life_leech_from_any_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_physical_damage_permyriad ) ) +
				stats.GetStat( base_life_leech_from_physical_damage_permyriad );
		}

		VIRTUAL_STAT( life_leech_from_fire_damage_permyriad,
			base_life_leech_from_fire_damage_permyriad,
			old_do_not_use_base_life_leech_from_fire_damage_permyriad,
			base_life_leech_from_elemental_damage_permyriad,
			old_do_not_use_base_life_leech_from_elemental_damage_permyriad,
			life_leech_from_any_damage_permyriad,
			life_leech_from_fire_damage_while_ignited_permyriad, is_ignited )
		{
			return stats.GetStat( life_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_life_leech_from_elemental_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_elemental_damage_permyriad ) ) +
				stats.GetStat( base_life_leech_from_fire_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_fire_damage_permyriad ) ) +
				( stats.GetStat( is_ignited ) ? stats.GetStat( life_leech_from_fire_damage_while_ignited_permyriad ) : 0 );
		}

		VIRTUAL_STAT( life_leech_from_cold_damage_permyriad,
			base_life_leech_from_cold_damage_permyriad,
			old_do_not_use_base_life_leech_from_cold_damage_permyriad,
			base_life_leech_from_elemental_damage_permyriad,
			old_do_not_use_base_life_leech_from_elemental_damage_permyriad,
			life_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( life_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_life_leech_from_elemental_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_elemental_damage_permyriad ) ) +
				stats.GetStat( base_life_leech_from_cold_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_cold_damage_permyriad ) );
		}

		VIRTUAL_STAT( life_leech_from_lightning_damage_permyriad,
			base_life_leech_from_lightning_damage_permyriad,
			old_do_not_use_base_life_leech_from_lightning_damage_permyriad,
			base_life_leech_from_elemental_damage_permyriad,
			old_do_not_use_base_life_leech_from_elemental_damage_permyriad,
			life_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( life_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_life_leech_from_elemental_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_elemental_damage_permyriad ) ) +
				stats.GetStat( base_life_leech_from_lightning_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_life_leech_from_lightning_damage_permyriad ) );
		}

		VIRTUAL_STAT( life_leech_from_chaos_damage_permyriad,
			base_life_leech_from_chaos_damage_permyriad,
			life_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( life_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_life_leech_from_chaos_damage_permyriad );
		}

		VIRTUAL_STAT( mana_leech_from_physical_damage_permyriad,
			base_mana_leech_from_physical_damage_permyriad,
			virtual_mana_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( virtual_mana_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_physical_damage_permyriad );
		}

		VIRTUAL_STAT( mana_leech_from_fire_damage_permyriad,
			base_mana_leech_from_fire_damage_permyriad,
			base_mana_leech_from_elemental_damage_permyriad,
			virtual_mana_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( virtual_mana_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_elemental_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_fire_damage_permyriad );
		}

		VIRTUAL_STAT( mana_leech_from_cold_damage_permyriad,
			base_mana_leech_from_cold_damage_permyriad,
			base_mana_leech_from_elemental_damage_permyriad,
			virtual_mana_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( virtual_mana_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_elemental_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_cold_damage_permyriad );
		}

		VIRTUAL_STAT( mana_leech_from_lightning_damage_permyriad,
			base_mana_leech_from_lightning_damage_permyriad,
			old_do_not_use_base_mana_leech_from_lightning_damage_permyriad,
			base_mana_leech_from_elemental_damage_permyriad,
			virtual_mana_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( virtual_mana_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_elemental_damage_permyriad ) +
				ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_base_mana_leech_from_lightning_damage_permyriad ) ) +
				stats.GetStat( base_mana_leech_from_lightning_damage_permyriad );
		}

		VIRTUAL_STAT( mana_leech_from_chaos_damage_permyriad,
			base_mana_leech_from_chaos_damage_permyriad,
			virtual_mana_leech_from_any_damage_permyriad )
		{
			return stats.GetStat( virtual_mana_leech_from_any_damage_permyriad ) +
				stats.GetStat( base_mana_leech_from_chaos_damage_permyriad );
		}

		VIRTUAL_STAT( virtual_mana_leech_from_any_damage_permyriad,
			mana_leech_from_any_damage_permyriad,
			mana_leech_from_any_damage_permyriad_with_all_corrupted_equipped_items, number_of_equipped_corrupted_items, number_of_equipped_items )
		{
			return stats.GetStat( mana_leech_from_any_damage_permyriad ) +
				( stats.GetStat( number_of_equipped_corrupted_items ) == stats.GetStat( number_of_equipped_items ) ?
					stats.GetStat( mana_leech_from_any_damage_permyriad_with_all_corrupted_equipped_items ) : 0 );
		}

		VIRTUAL_STAT( fire_damage_can_shock,
			unique_fire_damage_shocks )
		{
			return stats.GetStat( unique_fire_damage_shocks );
		}

		VIRTUAL_STAT( cold_damage_can_ignite,
			unique_cold_damage_ignites,
			unique_cold_damage_can_also_ignite )
		{
			return stats.GetStat( unique_cold_damage_ignites ) ||
				stats.GetStat( unique_cold_damage_can_also_ignite );
		}

		VIRTUAL_STAT( lightning_damage_can_freeze,
			unique_lightning_damage_freezes )
		{
			return stats.GetStat( unique_lightning_damage_freezes );
		}

		VIRTUAL_STAT( fire_damage_cannot_ignite,
			unique_fire_damage_shocks )
		{
			return stats.GetStat( unique_fire_damage_shocks );
		}

		VIRTUAL_STAT( cold_damage_cannot_freeze,
			unique_cold_damage_ignites )
		{
			return stats.GetStat( unique_cold_damage_ignites );
		}

		VIRTUAL_STAT( cold_damage_cannot_chill,
			unique_cold_damage_ignites )
		{
			return stats.GetStat( unique_cold_damage_ignites );
		}

		VIRTUAL_STAT( lightning_damage_cannot_shock,
			unique_lightning_damage_freezes )
		{
			return stats.GetStat( unique_lightning_damage_freezes );
		}

		VIRTUAL_STAT( fire_damage_taken_percent_as_chaos,
			elemental_damage_taken_percent_as_chaos )
		{
			return stats.GetStat( elemental_damage_taken_percent_as_chaos );
		}

		VIRTUAL_STAT( cold_damage_taken_percent_as_chaos,
			elemental_damage_taken_percent_as_chaos )
		{
			return stats.GetStat( elemental_damage_taken_percent_as_chaos );
		}

		VIRTUAL_STAT( lightning_damage_taken_percent_as_chaos,
			elemental_damage_taken_percent_as_chaos )
		{
			return stats.GetStat( elemental_damage_taken_percent_as_chaos );
		}

		VIRTUAL_STAT( life_gained_on_enemy_death,
			base_life_gained_on_enemy_death,
			life_gained_on_enemy_death_per_frenzy_charge, current_frenzy_charges,
			life_gained_on_enemy_death_per_level, level,
			cannot_recover_life )
		{
			// If we cannot regain life, exit
			if ( stats.GetStat( cannot_recover_life ) )
				return 0;

			return stats.GetStat( base_life_gained_on_enemy_death ) +
				stats.GetStat( life_gained_on_enemy_death_per_frenzy_charge ) * stats.GetStat( current_frenzy_charges ) +
				stats.GetStat( life_gained_on_enemy_death_per_level ) * stats.GetStat( level );
		}

		VIRTUAL_STAT( mana_gained_on_enemy_death,
			base_mana_gained_on_enemy_death,
			mana_gained_on_enemy_death_per_level, level )
		{
			return stats.GetStat( base_mana_gained_on_enemy_death ) +
				stats.GetStat( mana_gained_on_enemy_death_per_level ) * stats.GetStat( level );
		}

		VIRTUAL_STAT( energy_shield_gained_on_enemy_death,
			base_energy_shield_gained_on_enemy_death,
			energy_shield_gained_on_enemy_death_per_level, level )
		{
			return stats.GetStat( base_energy_shield_gained_on_enemy_death ) +
				stats.GetStat( energy_shield_gained_on_enemy_death_per_level ) * stats.GetStat( level );
		}

		VIRTUAL_STAT( skill_is_totemified,
			base_skill_is_totemified )
		{
			return stats.GetStat( base_skill_is_totemified );
		}

		VIRTUAL_STAT( skill_is_trapped,
			base_skill_is_trapped,
			base_skill_is_totemified )
		{
			return stats.GetStat( base_skill_is_trapped ) && !stats.GetStat( base_skill_is_totemified );
		}

		VIRTUAL_STAT( skill_is_mined,
			base_skill_is_mined,
			base_skill_is_trapped,
			base_skill_is_totemified )
		{
			return stats.GetStat( base_skill_is_mined ) && !stats.GetStat( base_skill_is_trapped ) && !stats.GetStat( base_skill_is_totemified );
		}

		VIRTUAL_STAT( main_hand_steal_power_frenzy_endurance_charges_on_hit_percent,
			base_steal_power_frenzy_endurance_charges_on_hit_percent,
			claw_steal_power_frenzy_endurance_charges_on_hit_percent, main_hand_weapon_type,
			bow_steal_power_frenzy_endurance_charges_on_hit_percent,
			main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool main_hand_all_1h_weapons_count = !!stats.GetStat( main_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int main_hand_weapon_index = stats.GetStat( main_hand_weapon_type );

			const bool is_using_claw = main_hand_all_1h_weapons_count || main_hand_weapon_index == Items::Claw;
			const bool is_using_bow = main_hand_weapon_index == Items::Bow;
			return ( is_using_claw ? stats.GetStat( claw_steal_power_frenzy_endurance_charges_on_hit_percent ) : 0 ) +
				( is_using_bow ? stats.GetStat( bow_steal_power_frenzy_endurance_charges_on_hit_percent ) : 0 ) +
				stats.GetStat( base_steal_power_frenzy_endurance_charges_on_hit_percent );
		}

		VIRTUAL_STAT( off_hand_steal_power_frenzy_endurance_charges_on_hit_percent,
			base_steal_power_frenzy_endurance_charges_on_hit_percent,
			claw_steal_power_frenzy_endurance_charges_on_hit_percent, off_hand_weapon_type,
			off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types )
		{
			const bool off_hand_all_1h_weapons_count = !!stats.GetStat( off_hand_varunastra_weapon_counts_as_all_1h_melee_weapon_types );
			const int off_hand_weapon_index = stats.GetStat( off_hand_weapon_type );

			const bool is_using_claw = off_hand_all_1h_weapons_count || off_hand_weapon_index == Items::Claw;
			return ( is_using_claw ? stats.GetStat( claw_steal_power_frenzy_endurance_charges_on_hit_percent ) : 0 ) +
				stats.GetStat( base_steal_power_frenzy_endurance_charges_on_hit_percent );
		}

		VIRTUAL_STAT( virtual_minion_damage_pluspercent,
			minion_damage_pluspercent,
			minion_damage_pluspercent_per_10_rampage_stacks, current_rampage_stacks,
			minion_damage_pluspercent_per_active_spectre, number_of_active_spectres,
			minion_damage_pluspercent_per_10_dex, dexterity,
			minion_damage_pluspercent_if_enemy_hit_recently, have_hit_an_enemy_recently )
		{
			return stats.GetStat( minion_damage_pluspercent ) +
				stats.GetStat( minion_damage_pluspercent_per_10_rampage_stacks ) * ( stats.GetStat( current_rampage_stacks ) / 20 ) + //Actually per 20
				stats.GetStat( minion_damage_pluspercent_per_active_spectre ) * ( stats.GetStat( number_of_active_spectres ) ) +
				stats.GetStat( minion_damage_pluspercent_per_10_dex ) * ( stats.GetStat( dexterity ) / 10 ) +
				( stats.GetStat( have_hit_an_enemy_recently ) ? stats.GetStat( minion_damage_pluspercent_if_enemy_hit_recently ) : 0 );
		}

		VIRTUAL_STAT( virtual_minion_movement_velocity_pluspercent,
			minion_movement_speed_pluspercent,
			minion_movement_velocity_pluspercent_per_10_rampage_stacks, current_rampage_stacks,
			movement_speed_increases_and_reductions_also_affects_your_minions, movement_velocity_pluspercent )
		{
			return	stats.GetStat( minion_movement_speed_pluspercent ) +
				( stats.GetStat( movement_speed_increases_and_reductions_also_affects_your_minions ) ? stats.GetStat( movement_velocity_pluspercent ) : 0 ) +
				( stats.GetStat( minion_movement_velocity_pluspercent_per_10_rampage_stacks ) * ( stats.GetStat( current_rampage_stacks ) / 20 ) ); //Actually per 20
		}

		VIRTUAL_STAT( skill_number_of_triggers
			, skill_triggerable_spell, casting_spell
			, cast_on_damage_taken_percent
			, cast_on_any_damage_taken_percent
			, cast_on_death_percent
			, chance_to_cast_on_kill_percent
			, cast_on_stunned_percent
			, cast_linked_spells_on_attack_crit_percent
			, cast_linked_spells_on_attack_hit_percent
			, cast_linked_spells_on_melee_kill_percent
			, unique_mjolner_lightning_spells_triggered
			, skill_is_curse
			, apply_linked_curses_on_hit_percent
			, melee_counterattack_trigger_on_block_percent, attack_is_melee
			, unique_bow_minion_spells_triggered
			, attack_trigger_when_critically_hit_percent
			, cast_when_critically_hit_percent
			, attack_trigger_on_hit_percent
			, cast_on_hit_percent
			, attack_trigger_on_kill_percent
			, chance_to_counter_strike_when_hit_percent
			, melee_counterattack_trigger_on_hit_percent
			, chance_to_cast_on_rampage_tier_percent
			, support_cast_on_mana_spent
			, chance_to_cast_on_owned_kill_percent
			, skill_triggerable_attack
			, attack_trigger_on_melee_hit_percent 
			, unique_cospris_malice_cold_spells_triggered )
		{
			unsigned num_triggers = 0;

			if ( stats.GetStat( skill_triggerable_spell ) )
			{
				if( stats.GetStat( cast_on_damage_taken_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_on_any_damage_taken_percent ) )
					num_triggers++;
				if( stats.GetStat( chance_to_cast_on_kill_percent ) )
					num_triggers++;
				if ( stats.GetStat( chance_to_cast_on_owned_kill_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_on_death_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_on_stunned_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_when_critically_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_on_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_linked_spells_on_attack_crit_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_linked_spells_on_attack_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( cast_linked_spells_on_melee_kill_percent ) )
					num_triggers++;
				if( stats.GetStat( unique_mjolner_lightning_spells_triggered ) ) //we rely on the support effect limiting which skills are supported, rather than having a stat specifically for that
					num_triggers++;
				if( stats.GetStat( unique_bow_minion_spells_triggered ) ) //we rely on the support effect limiting which skills are supported, rather than having a stat specifically for that
					num_triggers++;
				if( stats.GetStat( support_cast_on_mana_spent ) ) //we rely on the support effect limiting which skills are supported, rather than having a stat specifically for that
					num_triggers++;
				if( stats.GetStat( unique_cospris_malice_cold_spells_triggered ) )
					num_triggers++;
				if( stats.GetStat( chance_to_cast_on_rampage_tier_percent ) )
					num_triggers++;
			}

			if( stats.GetStat( skill_is_curse ) )
			{
				if( stats.GetStat( apply_linked_curses_on_hit_percent ) )
					num_triggers++;
			}

			if( stats.GetStat( skill_triggerable_attack ) ) //melee_ ones should be limited by attack_is_melee, but are not currently as some were re-used for enchantments
			{
				if( stats.GetStat( melee_counterattack_trigger_on_block_percent ) )
					num_triggers++;
				if( stats.GetStat( attack_trigger_when_critically_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( attack_trigger_on_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( attack_trigger_on_kill_percent ) )
					num_triggers++;
				if( stats.GetStat( chance_to_counter_strike_when_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( melee_counterattack_trigger_on_hit_percent ) )
					num_triggers++;
				if( stats.GetStat( attack_is_melee ) && stats.GetStat( attack_trigger_on_melee_hit_percent ) )
					num_triggers++;
			}

			return num_triggers;
		}

		VIRTUAL_STAT( skill_is_triggered,
			skill_number_of_triggers )
		{
			return stats.GetStat( skill_number_of_triggers ) == 1;
		}

		VIRTUAL_STAT( virtual_maximum_endurance_charges,
			max_endurance_charges )
		{
			return std::max( stats.GetStat( max_endurance_charges ), 0 );
		}

		VIRTUAL_STAT( virtual_maximum_frenzy_charges,
			max_frenzy_charges )
		{
			return std::max( stats.GetStat( max_frenzy_charges ), 0 );
		}

		VIRTUAL_STAT( virtual_maximum_power_charges,
			max_power_charges,
			no_maximum_power_charges )
		{
			if ( stats.GetStat( no_maximum_power_charges ) )
				return 0;

			return std::max( stats.GetStat( max_power_charges ), 0 );
		}

		VIRTUAL_STAT( virtual_ignite_duration_pluspercent,
			ignite_duration_pluspercent,
			base_elemental_status_ailment_duration_pluspercent )
		{
			return stats.GetStat( ignite_duration_pluspercent ) +
				stats.GetStat( base_elemental_status_ailment_duration_pluspercent );
		}

		VIRTUAL_STAT( virtual_shock_duration_pluspercent,
			shock_duration_pluspercent,
			base_elemental_status_ailment_duration_pluspercent )
		{
			return stats.GetStat( shock_duration_pluspercent ) +
				stats.GetStat( base_elemental_status_ailment_duration_pluspercent );
		}

		VIRTUAL_STAT( virtual_freeze_duration_pluspercent,
			freeze_duration_pluspercent,
			base_elemental_status_ailment_duration_pluspercent )
		{
			return stats.GetStat( freeze_duration_pluspercent ) +
				stats.GetStat( base_elemental_status_ailment_duration_pluspercent );
		}

		VIRTUAL_STAT( virtual_chill_duration_pluspercent,
			chill_duration_pluspercent,
			base_elemental_status_ailment_duration_pluspercent )
		{
			return stats.GetStat( chill_duration_pluspercent ) +
				stats.GetStat( base_elemental_status_ailment_duration_pluspercent );
		}

		VIRTUAL_STAT( virtual_power_charge_duration_pluspercent,
			power_charge_duration_pluspercent,
			charge_duration_pluspercent )
		{
			return stats.GetStat( power_charge_duration_pluspercent ) +
				stats.GetStat( charge_duration_pluspercent );
		}

		VIRTUAL_STAT( virtual_endurance_charge_duration_pluspercent,
			endurance_charge_duration_pluspercent,
			charge_duration_pluspercent )
		{
			return stats.GetStat( endurance_charge_duration_pluspercent ) +
				stats.GetStat( charge_duration_pluspercent );
		}

		VIRTUAL_STAT( fire_damage_heals,
			base_elemental_damage_heals,
			base_fire_damage_heals )
		{
			return stats.GetStat( base_elemental_damage_heals ) || stats.GetStat( base_fire_damage_heals );
		}

		VIRTUAL_STAT( cold_damage_heals,
			base_elemental_damage_heals,
			base_cold_damage_heals )
		{
			return stats.GetStat( base_elemental_damage_heals ) || stats.GetStat( base_cold_damage_heals );
		}

		VIRTUAL_STAT( lightning_damage_heals,
			base_elemental_damage_heals,
			base_lightning_damage_heals )
		{
			return stats.GetStat( base_elemental_damage_heals ) || stats.GetStat( base_lightning_damage_heals );
		}
		
		VIRTUAL_STAT( is_leeching,
			is_life_leeching,
			is_mana_leeching,
			is_es_leeching )
		{
			return stats.GetStat( is_life_leeching ) || stats.GetStat( is_mana_leeching ) || stats.GetStat( is_es_leeching );

		}

		VIRTUAL_STAT( virtual_number_of_additional_projectiles,
			number_of_additional_projectiles,
			totem_number_of_additional_projectiles, skill_is_totemified,
			attacks_number_of_additional_projectiles, skill_is_attack,
			casting_spell, spells_number_of_additional_projectiles )
		{
			return stats.GetStat( number_of_additional_projectiles ) +
				( stats.GetStat( skill_is_attack ) ? stats.GetStat( attacks_number_of_additional_projectiles ) : 0 ) +
				( stats.GetStat( skill_is_totemified ) ? stats.GetStat( totem_number_of_additional_projectiles ) : 0 ) +
				( stats.GetStat( casting_spell ) ? stats.GetStat( spells_number_of_additional_projectiles ) : 0 );
		}

		VIRTUAL_STAT( virtual_number_of_additional_projectiles_in_chain,
			number_of_additional_projectiles_in_chain,
			skill_is_attack, attacks_num_of_additional_chains,
			num_of_additional_chains_at_max_frenzy_charges, current_frenzy_charges, virtual_maximum_frenzy_charges )
		{
			return stats.GetStat( number_of_additional_projectiles_in_chain ) +
				( stats.GetStat( skill_is_attack ) ? stats.GetStat( attacks_num_of_additional_chains ) : 0 ) +
				( stats.GetStat( current_frenzy_charges ) == stats.GetStat( virtual_maximum_frenzy_charges ) ? stats.GetStat( num_of_additional_chains_at_max_frenzy_charges ) : 0 );
		}

		VIRTUAL_STAT( virtual_firestorm_drop_chilled_ground_duration_ms,
			firestorm_drop_ground_ice_duration_ms,
			skill_effect_duration_pluspercent )
		{
			return Round( stats.GetStat( firestorm_drop_ground_ice_duration_ms ) *	Scale( 100 + stats.GetStat( skill_effect_duration_pluspercent ) ) );
		}

		VIRTUAL_STAT( virtual_firestorm_drop_shocked_ground_duration_ms,
			firestorm_drop_ground_shock_duration_ms,
			skill_effect_duration_pluspercent )
		{
			return Round( stats.GetStat( firestorm_drop_ground_shock_duration_ms ) *	Scale( 100 + stats.GetStat( skill_effect_duration_pluspercent ) ) );
		}

		VIRTUAL_STAT( virtual_firestorm_drop_burning_ground_duration_ms,
			firestorm_drop_burning_ground_duration_ms,
			skill_effect_duration_pluspercent )
		{
			return Round( stats.GetStat( firestorm_drop_burning_ground_duration_ms ) *	Scale( 100 + stats.GetStat( skill_effect_duration_pluspercent ) ) );
		}

		VIRTUAL_STAT( virtual_life_leech_speed_pluspercent,
			life_leech_speed_pluspercent,
			life_leech_speed_pluspercent_per_equipped_corrupted_item, number_of_equipped_corrupted_items )
		{
			return stats.GetStat( life_leech_speed_pluspercent ) +
				( stats.GetStat( life_leech_speed_pluspercent_per_equipped_corrupted_item ) * stats.GetStat( number_of_equipped_corrupted_items ) );
		}

		VIRTUAL_STAT( virtual_mana_leech_speed_pluspercent,
			mana_leech_speed_pluspercent,
			mana_leech_speed_pluspercent_per_equipped_corrupted_item, number_of_equipped_corrupted_items )
		{
			return stats.GetStat( mana_leech_speed_pluspercent ) +
				( stats.GetStat( mana_leech_speed_pluspercent_per_equipped_corrupted_item ) * stats.GetStat( number_of_equipped_corrupted_items ) );
		}

		VIRTUAL_STAT( chaos_damage_damages_energy_shield_percent,
			half_physical_bypasses_es_half_chaos_damages_es_when_X_corrupted_items_equipped, number_of_equipped_corrupted_items,
			chaos_damage_does_not_bypass_energy_shield )
		{
			if ( stats.GetStat( chaos_damage_does_not_bypass_energy_shield ) )
			{
				return 100;
			}
			else if ( const auto items_required = stats.GetStat( half_physical_bypasses_es_half_chaos_damages_es_when_X_corrupted_items_equipped ) )
			{
				if ( items_required <= stats.GetStat( number_of_equipped_corrupted_items ) )
				{
					return 50;
				}
			}

			// Chaos bypasses es by default 
			return 0;

		}

		VIRTUAL_STAT( physical_damage_bypass_energy_shield_percent,
			half_physical_bypasses_es_half_chaos_damages_es_when_X_corrupted_items_equipped, number_of_equipped_corrupted_items )
		{
			if ( const auto items_required = stats.GetStat( half_physical_bypasses_es_half_chaos_damages_es_when_X_corrupted_items_equipped ) )
			{
				if ( items_required <= stats.GetStat( number_of_equipped_corrupted_items ) )
				{
					return 50;
				}
			}

			// Phys doesnt bypass es by default
			return 0;

		}

		VIRTUAL_STAT( virtual_action_speed_pluspercent,
			temporal_chains_action_speed_pluspercent_final,
			action_speed_minuspercent,
			action_speed_pluspercent_while_chilled, is_chilled,
			action_speed_cannot_be_reduced_below_base,
			is_petrified )
		{
			if ( stats.GetStat( is_petrified ) > 0 )
				return -100;

			const int temp_chains_action_speed = std::max( stats.GetStat( temporal_chains_action_speed_pluspercent_final ), -75 );
			const int result = Round( ( 100 -
				stats.GetStat( action_speed_minuspercent ) +
				( stats.GetStat( is_chilled ) ? stats.GetStat( action_speed_pluspercent_while_chilled ) : 0 ) )
				* Scale( 100 + temp_chains_action_speed ) - 100 );

			return ( stats.GetStat( action_speed_cannot_be_reduced_below_base ) && result < 0 ) ? 0 : result;
		}

		VIRTUAL_STAT( virtual_cooldown_speed_pluspercent
			, base_cooldown_speed_pluspercent
			, is_warcry, warcry_cooldown_speed_pluspercent
			, placing_traps_cooldown_recovery_pluspercent, base_skill_is_trapped, base_skill_is_mined
			, blink_arrow_cooldown_speed_pluspercent, active_skill_index
			, mirror_arrow_cooldown_speed_pluspercent
			)
		{
			const bool skill_throws_trap = !!stats.GetStat( base_skill_is_trapped ) && !stats.GetStat( base_skill_is_mined );
			return stats.GetStat( base_cooldown_speed_pluspercent ) +
				( stats.GetStat( is_warcry ) ? stats.GetStat( warcry_cooldown_speed_pluspercent ) : 0 ) +
				( skill_throws_trap ? stats.GetStat( placing_traps_cooldown_recovery_pluspercent ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::blink_arrow ) ? stats.GetStat( blink_arrow_cooldown_speed_pluspercent ) : 0 ) +
				( ( stats.GetStat( active_skill_index ) == Loaders::ActiveSkillsValues::mirror_arrow ) ? stats.GetStat( mirror_arrow_cooldown_speed_pluspercent ) : 0 );
		}

		//used by viper strike to override the standard base poison duration.
		VIRTUAL_STAT( poison_skill_effect_duration,
			base_skill_effect_duration,
			virtual_skill_effect_duration_pluspercent_final,
			poison_duration_pluspercent )
		{
			return Round( ( stats.GetStat( base_skill_effect_duration ) ) *
				Scale( 100 + stats.GetStat( poison_duration_pluspercent ) ) *
				Scale( 100 + stats.GetStat( virtual_skill_effect_duration_pluspercent_final ) ) );
		}

		VIRTUAL_STAT( poison_duration_pluspercent,
			virtual_skill_effect_duration_pluspercent,
			buff_duration_pluspercent,
			buff_effect_duration_pluspercent_per_endurance_charge, current_endurance_charges,
			base_poison_duration_pluspercent )
		{
			return stats.GetStat( virtual_skill_effect_duration_pluspercent ) + stats.GetStat( buff_duration_pluspercent ) +
				( stats.GetStat( buff_effect_duration_pluspercent_per_endurance_charge ) * stats.GetStat( current_endurance_charges ) ) +
				stats.GetStat( base_poison_duration_pluspercent );
		}

		VIRTUAL_STAT( virtual_minion_additional_physical_damage_reduction_percent,
			minion_additional_physical_damage_reduction_percent,
			physical_damage_reduction_and_minion_physical_damage_reduction_percent_per_raised_zombie, number_of_active_zombies,
			physical_damage_reduction_and_minion_physical_damage_reduction_percent )
		{
			return stats.GetStat( minion_additional_physical_damage_reduction_percent ) +
				stats.GetStat( physical_damage_reduction_and_minion_physical_damage_reduction_percent ) +
				stats.GetStat( physical_damage_reduction_and_minion_physical_damage_reduction_percent_per_raised_zombie ) * stats.GetStat( number_of_active_zombies );
		}

		VIRTUAL_STAT( virtual_number_of_ranged_animated_weapons_allowed,
					  base_number_of_ranged_animated_weapons_allowed )
		{
			return
				stats.GetStat( base_number_of_ranged_animated_weapons_allowed );
		}

		VIRTUAL_STAT( virtual_player_gain_rampage_stacks,
			player_gain_rampage_stacks,
			current_rampage_stacks,
			gain_rampage_while_at_maximum_endurance_charges,
			current_endurance_charges, virtual_maximum_endurance_charges )
		{
			return stats.GetStat( player_gain_rampage_stacks ) || stats.GetStat( current_rampage_stacks ) > 0 ||
				( stats.GetStat( gain_rampage_while_at_maximum_endurance_charges ) ? stats.GetStat( current_endurance_charges ) == stats.GetStat( virtual_maximum_endurance_charges ) : false );
		}

		VIRTUAL_STAT( virtual_minion_attack_speed_pluspercent,
			minion_attack_speed_pluspercent,
			minion_attack_and_cast_speed_pluspercent_per_active_skeleton, number_of_active_skeletons )
		{
			return stats.GetStat( minion_attack_speed_pluspercent ) +
				stats.GetStat( minion_attack_and_cast_speed_pluspercent_per_active_skeleton ) * stats.GetStat( number_of_active_skeletons );
		}

		VIRTUAL_STAT( virtual_minion_cast_speed_pluspercent,
			minion_cast_speed_pluspercent,
			minion_attack_and_cast_speed_pluspercent_per_active_skeleton, number_of_active_skeletons )
		{
			return stats.GetStat( minion_cast_speed_pluspercent ) +
				stats.GetStat( minion_attack_and_cast_speed_pluspercent_per_active_skeleton ) * stats.GetStat( number_of_active_skeletons );
		}

		VIRTUAL_STAT( virtual_minion_life_regeneration_per_minute,
			minion_life_regeneration_per_minute_per_active_raging_spirit, number_of_active_raging_spirits )
		{
			return stats.GetStat( minion_life_regeneration_per_minute_per_active_raging_spirit ) * stats.GetStat( number_of_active_raging_spirits );
		}

		VIRTUAL_STAT( virtual_physical_damage_taken_pluspercent,
			physical_damage_taken_pluspercent,
			physical_damage_taken_pluspercent_while_frozen, is_frozen,
			physical_damage_taken_pluspercent_while_at_maximum_endurance_charges,
			current_endurance_charges, virtual_maximum_endurance_charges )
		{
			const auto max_endurance = stats.GetStat( current_endurance_charges ) == stats.GetStat( virtual_maximum_endurance_charges );

			return stats.GetStat( physical_damage_taken_pluspercent ) +
				( stats.GetStat( is_frozen ) ? stats.GetStat( physical_damage_taken_pluspercent_while_frozen ) : 0 ) +
				( max_endurance ? stats.GetStat( physical_damage_taken_pluspercent_while_at_maximum_endurance_charges ) : 0 );
		}

		VIRTUAL_STAT( virtual_elemental_damage_taken_pluspercent,
			elemental_damage_taken_pluspercent,
			elemental_damage_taken_pluspercent_at_maximum_endurance_charges, current_endurance_charges, virtual_maximum_endurance_charges,
			using_flask, elemental_damage_taken_pluspercent_during_flask_effect,
			elemental_damage_taken_pluspercent_while_on_consecrated_ground, on_consecrated_ground )
		{
			return stats.GetStat( elemental_damage_taken_pluspercent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( elemental_damage_taken_pluspercent_during_flask_effect ) : 0 ) +
				( stats.GetStat( on_consecrated_ground ) ? stats.GetStat( elemental_damage_taken_pluspercent_while_on_consecrated_ground ) : 0 ) +
				( stats.GetStat( current_endurance_charges ) >= stats.GetStat( virtual_maximum_endurance_charges ) ? stats.GetStat( elemental_damage_taken_pluspercent_at_maximum_endurance_charges ) : 0 );
		}

		VIRTUAL_STAT( virtual_aura_effect_pluspercent
			, aura_effect_pluspercent
			, non_curse_aura_effect_pluspercent, skill_is_curse
			)
		{
			return stats.GetStat( aura_effect_pluspercent ) +
				( stats.GetStat( skill_is_curse ) ? 0 : stats.GetStat( non_curse_aura_effect_pluspercent ) );
		}

		VIRTUAL_STAT( is_burning,
			base_fire_damage_taken_per_minute,
			base_fire_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute,
			base_nonlethal_fire_damage_percent_of_maximum_life_taken_per_minute,
			base_nonlethal_fire_damage_percent_of_maximum_energy_shield_taken_per_minute )
		{
			return ( stats.GetStat( base_fire_damage_taken_per_minute ) +
				stats.GetStat( base_fire_damage_percent_of_maximum_life_plus_maximum_ES_taken_per_minute ) +
				stats.GetStat( base_nonlethal_fire_damage_percent_of_maximum_life_taken_per_minute ) +
				stats.GetStat( base_nonlethal_fire_damage_percent_of_maximum_energy_shield_taken_per_minute ) ) > 0;
		}

		VIRTUAL_STAT( virtual_base_maximum_energy_shield_to_grant_to_you_and_nearby_allies,
			mana_reserved, guardian_reserved_mana_percent_given_to_you_and_nearby_allies_as_base_maximum_energy_shield )
		{
			unsigned out = 0;

			if ( const auto reserved_percent_to_grant = stats.GetStat( guardian_reserved_mana_percent_given_to_you_and_nearby_allies_as_base_maximum_energy_shield ) )
			{
				if ( const auto reserved_mana = stats.GetStat( mana_reserved ) )
				{
					out += ( unsigned ) ( ( float ) reserved_percent_to_grant / 100.0f * reserved_mana );
				}
			}

			return out;
		}

		VIRTUAL_STAT( virtual_armour_to_grant_to_you_and_nearby_allies,
			life_reserved, guardian_reserved_life_granted_to_you_and_allies_as_armour_percent )
		{
			unsigned out = 0;

			if ( const auto reserved_percent_to_grant = stats.GetStat( guardian_reserved_life_granted_to_you_and_allies_as_armour_percent ) )
			{
				if ( const auto reserved_life = stats.GetStat( life_reserved ) )
				{
					out += ( unsigned ) ( ( float ) reserved_percent_to_grant / 100.0f * reserved_life );
				}
			}

			return out;
		}

		VIRTUAL_STAT( virtual_chance_to_taunt_on_hit_percent,
			chance_to_taunt_on_hit_percent,
			totemified_skills_taunt_on_hit_percent, skill_is_totemified )
		{
			return stats.GetStat( chance_to_taunt_on_hit_percent ) +
				( stats.GetStat( skill_is_totemified ) ? stats.GetStat( totemified_skills_taunt_on_hit_percent ) : 0 );
		}

		VIRTUAL_STAT( chance_to_poison_on_hit_percent,
			base_chance_to_poison_on_hit_percent,
			poison_on_hit_during_flask_effect_percent, using_flask,
			skill_is_trapped, skill_is_mined, traps_and_mines_percent_chance_to_poison )
		{
			return stats.GetStat( base_chance_to_poison_on_hit_percent ) +
				( stats.GetStat( skill_is_trapped ) || stats.GetStat( skill_is_mined ) ? stats.GetStat( traps_and_mines_percent_chance_to_poison ) : 0 ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( poison_on_hit_during_flask_effect_percent ) : 0 );
		}

		VIRTUAL_STAT( flask_charges_gained_pluspercent,
			charges_gained_pluspercent,
			flask_charges_gained_pluspercent_during_flask_effect, using_flask )
		{
			return stats.GetStat( charges_gained_pluspercent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( flask_charges_gained_pluspercent_during_flask_effect ) : 0 );
		}

		VIRTUAL_STAT( soul_eater_from_stat,
			gain_soul_eater_during_flask_effect, using_flask )
		{
			return stats.GetStat( gain_soul_eater_during_flask_effect ) && stats.GetStat( using_flask );
		}

		VIRTUAL_STAT( virtual_energy_shield_delay_minuspercent,
			energy_shield_delay_minuspercent,
			energy_shield_delay_during_flask_effect_minuspercent,
			using_flask )
		{
			return stats.GetStat( energy_shield_delay_minuspercent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( energy_shield_delay_during_flask_effect_minuspercent ) : 0 );
		}

		VIRTUAL_STAT( virtual_energy_shield_recharge_rate_pluspercent,
			energy_shield_recharge_rate_pluspercent,
			energy_shield_recharge_rate_during_flask_effect_pluspercent,
			using_flask )
		{
			return stats.GetStat( energy_shield_recharge_rate_pluspercent ) +
				( stats.GetStat( using_flask ) ? stats.GetStat( energy_shield_recharge_rate_during_flask_effect_pluspercent ) : 0 );
		}

		VIRTUAL_STAT( virtual_elemental_status_effect_aura_radius,
			elemental_status_effect_aura_radius,
			elementalist_elemental_status_effect_aura_radius )
		{
			return std::max( stats.GetStat( elemental_status_effect_aura_radius ), stats.GetStat( elementalist_elemental_status_effect_aura_radius ) );
		}

		VIRTUAL_STAT( virtual_life_leech_permyriad_on_crit,
			life_leech_permyriad_on_crit,
			old_do_not_use_life_leech_permyriad_on_crit )
		{
			return stats.GetStat( life_leech_permyriad_on_crit ) + ScaleOldPermyriadLeech( stats.GetStat( old_do_not_use_life_leech_permyriad_on_crit ) );
		}

		VIRTUAL_STAT( is_counterattack,
			chance_to_counter_strike_when_hit_percent,
			melee_counterattack_trigger_on_block_percent,
			melee_counterattack_trigger_on_hit_percent )
		{
			return !!stats.GetStat( chance_to_counter_strike_when_hit_percent ) ||
				!!stats.GetStat( melee_counterattack_trigger_on_block_percent ) ||
				!!stats.GetStat( melee_counterattack_trigger_on_hit_percent );
		}

		VIRTUAL_STAT( main_hand_number_of_times_damage_is_doubled,
			global_number_of_times_damage_is_doubled,
			base_main_hand_number_of_times_damage_is_doubled )
		{
			return stats.GetStat( global_number_of_times_damage_is_doubled ) + stats.GetStat( base_main_hand_number_of_times_damage_is_doubled );
		}

		VIRTUAL_STAT( off_hand_number_of_times_damage_is_doubled,
			global_number_of_times_damage_is_doubled,
			base_off_hand_number_of_times_damage_is_doubled )
		{
			return stats.GetStat( global_number_of_times_damage_is_doubled ) + stats.GetStat( base_off_hand_number_of_times_damage_is_doubled );
		}

		VIRTUAL_STAT( virtual_pierce_percent,
			pierce_percent,
			always_pierce )
		{
			if( stats.GetStat( always_pierce ) )
				return 100;

			return stats.GetStat( pierce_percent );
		}

		VIRTUAL_STAT( virtual_phase_through_objects,
			phase_through_objects,
			gain_phasing_while_at_maximum_frenzy_charges,
			current_frenzy_charges, virtual_maximum_frenzy_charges,
			gain_phasing_while_you_have_onslaught, virtual_has_onslaught,
			gain_phasing_if_enemy_killed_recently, have_killed_in_past_4_seconds )
			{
				return stats.GetStat( phase_through_objects ) ||
					( stats.GetStat( current_frenzy_charges ) == stats.GetStat( virtual_maximum_frenzy_charges ) && stats.GetStat( gain_phasing_while_at_maximum_frenzy_charges ) ) ||
					( stats.GetStat( virtual_has_onslaught ) && stats.GetStat( gain_phasing_while_you_have_onslaught ) ) ||
					( stats.GetStat( gain_phasing_if_enemy_killed_recently ) && stats.GetStat( have_killed_in_past_4_seconds ) );
		}

		VIRTUAL_STAT( virtual_has_onslaught,
			has_onslaught,
			current_frenzy_charges, virtual_maximum_frenzy_charges,
			gain_onslaught_while_frenzy_charges_full )
		{
			return stats.GetStat( has_onslaught ) ||
				( stats.GetStat( current_frenzy_charges ) == stats.GetStat( virtual_maximum_frenzy_charges ) && stats.GetStat( gain_onslaught_while_frenzy_charges_full ) );
		}

		VIRTUAL_STAT( virtual_curse_effect_pluspercent,
			curse_effect_pluspercent )
		{
			return stats.GetStat( curse_effect_pluspercent );
		}

		VIRTUAL_STAT( track_have_killed_in_past_4_seconds
			, enchantment_boots_minimum_added_lightning_damage_when_you_havent_killed_for_4_seconds
			, enchantment_boots_maximum_added_lightning_damage_when_you_havent_killed_for_4_seconds
			, enchantment_boots_damage_penetrates_elemental_resistance_percent_while_you_havent_killed_for_4_seconds
			, damage_pluspercent_if_enemy_killed_recently_final
			, skill_area_of_effect_pluspercent_if_enemy_killed_recently
			, attack_speed_pluspercent_if_enemy_not_killed_recently
			, movement_speed_pluspercent_if_enemy_killed_recently
			, gain_phasing_if_enemy_killed_recently )
		{
			return !!stats.GetStat( enchantment_boots_minimum_added_lightning_damage_when_you_havent_killed_for_4_seconds ) ||
				!!stats.GetStat( enchantment_boots_maximum_added_lightning_damage_when_you_havent_killed_for_4_seconds ) ||
				!!stats.GetStat( enchantment_boots_damage_penetrates_elemental_resistance_percent_while_you_havent_killed_for_4_seconds ) ||
				!!stats.GetStat( damage_pluspercent_if_enemy_killed_recently_final ) ||
				!!stats.GetStat( skill_area_of_effect_pluspercent_if_enemy_killed_recently ) ||
				!!stats.GetStat( attack_speed_pluspercent_if_enemy_not_killed_recently ) ||
				!!stats.GetStat( movement_speed_pluspercent_if_enemy_killed_recently ) ||
				!!stats.GetStat( gain_phasing_if_enemy_killed_recently );
		}

		VIRTUAL_STAT( track_have_been_hit_in_past_4_seconds
			, enchantment_boots_movement_speed_pluspercent_when_not_hit_for_4_seconds 
			, damage_taken_pluspercent_if_not_hit_recently_final 
			, evasion_pluspercent_if_hit_recently )
		{
			return !!stats.GetStat( enchantment_boots_movement_speed_pluspercent_when_not_hit_for_4_seconds ) ||
				!!stats.GetStat( damage_taken_pluspercent_if_not_hit_recently_final ) ||
				!!stats.GetStat( evasion_pluspercent_if_hit_recently );
		}

		VIRTUAL_STAT( track_have_crit_in_past_4_seconds
			, enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds
			, enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds )
		{
			return !!stats.GetStat( enchantment_boots_status_ailment_chance_pluspercent_when_havent_crit_for_4_seconds ) ||
				!!stats.GetStat( enchantment_critical_strike_chance_pluspercent_if_you_havent_crit_for_4_seconds );
		}
		
		VIRTUAL_STAT( track_have_been_savage_hit_recently, 
			damage_taken_pluspercent_if_you_have_taken_a_savage_hit_recently )
		{
			return !!stats.GetStat( damage_taken_pluspercent_if_you_have_taken_a_savage_hit_recently );
		}

		VIRTUAL_STAT( track_have_taken_attack_damage_recently, 
			spell_dodge_chance_pluspercent_if_you_have_taken_attack_damage_recently )
		{
			return !!stats.GetStat( spell_dodge_chance_pluspercent_if_you_have_taken_attack_damage_recently );
		}

		VIRTUAL_STAT( track_have_used_a_warcry_recently,
			movement_speed_pluspercent_if_used_a_warcry_recently )
		{
			return !!stats.GetStat( movement_speed_pluspercent_if_used_a_warcry_recently );
		}

		VIRTUAL_STAT( track_have_killed_a_maimed_enemy_recently,
			skill_effect_duration_pluspercent_if_killed_maimed_enemy_recently )
		{
			return !!stats.GetStat( skill_effect_duration_pluspercent_if_killed_maimed_enemy_recently );
		}

		VIRTUAL_STAT( track_have_taunted_an_enemy_recently,
			damage_taken_pluspercent_if_taunted_an_enemy_recently,
			life_regeneration_rate_per_minute_percent_if_taunted_an_enemy_recently )
		{
			return !!stats.GetStat( damage_taken_pluspercent_if_taunted_an_enemy_recently ) ||
				!!stats.GetStat( life_regeneration_rate_per_minute_percent_if_taunted_an_enemy_recently );
		}

		VIRTUAL_STAT( track_have_taken_spell_damage_recently, 
			dodge_chance_pluspercent_if_you_have_taken_spell_damage_recently )
		{
			return !!stats.GetStat( dodge_chance_pluspercent_if_you_have_taken_spell_damage_recently );
		}

		VIRTUAL_STAT( track_have_pierced_recently,
			movement_speed_pluspercent_if_pierced_recently )
		{
			return !!stats.GetStat( movement_speed_pluspercent_if_pierced_recently );
		}
		
		VIRTUAL_STAT( track_number_of_corpses_consumed_recently,
			damage_pluspercent_if_you_have_consumed_a_corpse_recently, 
			attack_and_cast_speed_pluspercent_per_corpse_consumed_recently )
		{
			return !!stats.GetStat( damage_pluspercent_if_you_have_consumed_a_corpse_recently ) ||
				!!stats.GetStat( attack_and_cast_speed_pluspercent_per_corpse_consumed_recently );
		}

		VIRTUAL_STAT( track_have_blocked_recently,
			minimum_added_fire_damage_if_blocked_recently,
			maximum_added_fire_damage_if_blocked_recently )
		{
			return !!stats.GetStat( minimum_added_fire_damage_if_blocked_recently ) ||
				!!stats.GetStat( maximum_added_fire_damage_if_blocked_recently );
		}

		VIRTUAL_STAT( track_have_hit_an_enemy_recently,
			minion_damage_pluspercent_if_enemy_hit_recently )
		{
			return !!stats.GetStat( minion_damage_pluspercent_if_enemy_hit_recently );
		}

		VIRTUAL_STAT( virtual_reduce_enemy_cold_resistance_with_weapons_percent
			, reduce_enemy_cold_resistance_with_weapons_percent
			, reduce_enemy_elemental_resistance_with_weapons_percent )
		{
			return stats.GetStat( reduce_enemy_cold_resistance_with_weapons_percent ) + stats.GetStat( reduce_enemy_elemental_resistance_with_weapons_percent );
		}

		VIRTUAL_STAT( virtual_reduce_enemy_fire_resistance_with_weapons_percent
			, reduce_enemy_fire_resistance_with_weapons_percent
			, reduce_enemy_elemental_resistance_with_weapons_percent )
		{
			return stats.GetStat( reduce_enemy_fire_resistance_with_weapons_percent ) + stats.GetStat( reduce_enemy_elemental_resistance_with_weapons_percent );
		}

		VIRTUAL_STAT( virtual_reduce_enemy_lightning_resistance_with_weapons_percent
			, reduce_enemy_lightning_resistance_with_weapons_percent
			, reduce_enemy_elemental_resistance_with_weapons_percent )
		{
			return stats.GetStat( reduce_enemy_lightning_resistance_with_weapons_percent ) + stats.GetStat( reduce_enemy_elemental_resistance_with_weapons_percent );
		}

		VIRTUAL_STAT( virtual_current_number_of_golem_types
			, current_number_of_chaos_golems
			, current_number_of_fire_golems
			, current_number_of_ice_golems
			, current_number_of_lightning_golems
			, current_number_of_stone_golems )
		{
			return std::min( 1, stats.GetStat( current_number_of_chaos_golems ) ) +
				std::min( 1, stats.GetStat( current_number_of_fire_golems ) ) +
				std::min( 1, stats.GetStat( current_number_of_ice_golems ) ) +
				std::min( 1, stats.GetStat( current_number_of_lightning_golems ) ) +
				std::min( 1, stats.GetStat( current_number_of_stone_golems ) );
		}
	}
}