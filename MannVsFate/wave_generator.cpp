#include "wave_generator.h"
#include "rand_util.h"
#include "random_name_generator.h"
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <iostream>

void wave_generator::set_map_name(const std::string& in)
{
	map_name = in;
}

void wave_generator::set_mission_name(const std::string& in)
{
	mission_name = in;
}

void wave_generator::set_starting_currency(int in)
{
	starting_currency = in;
}

void wave_generator::set_waves(int in)
{
	waves = in;
}

void wave_generator::set_respawn_wave_time(int in)
{
	respawn_wave_time = in;
}

void wave_generator::set_event_popfile(int in)
{
	event_popfile = in;
}

void wave_generator::set_fixed_respawn_wave_time(bool in)
{
	fixed_respawn_wave_time = in;
}

void wave_generator::set_add_sentry_buster_when_damage_dealt_exceeds(int in)
{
	add_sentry_buster_when_damage_dealt_exceeds = in;
}

void wave_generator::set_add_sentry_buster_when_kill_count_exceeds(int in)
{
	add_sentry_buster_when_kill_count_exceeds = in;
}

void wave_generator::set_can_bots_attack_while_in_spawn_room(int in)
{
	can_bots_attack_while_in_spawn_room = in;
}

void wave_generator::set_sentry_buster_cooldown(float in)
{
	sentry_buster_cooldown = in;
}

void wave_generator::set_players(int in)
{
	players = in;
}

void wave_generator::set_currency_per_wave(int in)
{
	currency_per_wave = in;
}

void wave_generator::set_max_time(int in)
{
	max_time = in;
}

void wave_generator::set_tank_chance(float in)
{
	tank_chance = in;
}

void wave_generator::set_max_icons(int in)
{
	max_icons = in;
}

void wave_generator::set_max_wavespawns(int in)
{
	max_wavespawns = in;
}

void wave_generator::set_possible_classes(const std::vector<player_class>& classes)
{
	botgen.set_possible_classes(classes);
}

void wave_generator::set_max_tfbot_wavespawn_time(int in)
{
	max_tfbot_wavespawn_time = in;
}

void wave_generator::generate_mission(int argc, char** argv)
{
	std::stringstream filename;
	filename << map_name << '_' << players << "p_" << mission_name << ".pop";
	popfile.open(filename.str());
	if (!popfile)
	{
		const std::string exstr = "wave_generator::generate_mission exception: Couldn't write to file \"" + filename.str() + "\".";
		throw std::exception(exstr.c_str());
	}

	// Generate a bit of nice info at the top of the mission file.
	write("// This mission file was randomly generated by Mann Vs Fate version " + version + '.');
	if (argc > 1)
	{
		write("// The following command line arguments were used:");
		popfile << "// ";
		for (int i = 1; i < argc; ++i)
		{
			popfile << argv[i] << ' ';
		}
		popfile << '\n';
	}
	write_blank();
	write("#base robot_giant.pop"); // For the Sentry Buster template.
	write_blank();
	block_start("WaveSchedule");

	// WaveSchedule universal options.
	write("StartingCurrency", starting_currency);
	write("RespawnWaveTime", respawn_wave_time);
	if (event_popfile != 0)
	{
		write("EventPopfile", event_popfile);
	}
	if (fixed_respawn_wave_time)
	{
		write("FixedRespawnWaveTime", 1);
	}
	write("AddSentryBusterWhenDamageDealtExceeds", add_sentry_buster_when_damage_dealt_exceeds);
	write("AddSentryBusterWhenKillCountExceeds", add_sentry_buster_when_kill_count_exceeds);
	if (can_bots_attack_while_in_spawn_room == 0)
	{
		write("CanBotsAttackWhileInSpawnRoom", "no");
	}
	else
	{
		write("CanBotsAttackWhileInSpawnRoom", "yes");
	}
	write_blank();
	// Sentry Buster Mission.
	block_start("Mission");
	write("Objective", "DestroySentries");
	write("InitialCooldown", 5);
	write("Where", "spawnbot");
	write("BeginAtWave", 1);
	write("RunForThisManyWaves", waves);
	write("CooldownTime", sentry_buster_cooldown);
	block_start("TFBot");
	write("Template", "T_TFBot_SentryBuster");
	block_end(); // TFBot
	block_end(); // Mission

	// How much currency the players currently have.
	int current_currency = starting_currency;

	// Generate the actual waves!
	while (current_wave < waves)
	{
		++current_wave;

		std::cout << "Generating wave " << current_wave << '/' << waves << '.' << std::endl;

		write_indents();
		popfile << "// WAVE " << current_wave <<
			" ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n";
		block_start("Wave");
		block_start("StartWaveOutput");
		write("Target", "wave_start_relay");
		write("Action", "Trigger");
		block_end(); // StartWaveOutput
		block_start("DoneOutput");
		write("Target", "wave_finished_relay");
		write("Action", "Trigger");
		block_end(); // DoneOutPut

		std::cout << "Generated wave header, starting actual wavespawn generation..." << std::endl;

		// The actual WaveSpawn generation starts here!

		// Our current position in time in the wave as we walk through the wave.
		// We will use this to determine each WaveSpawn's WaitBeforeStarting value.
		int t = 0;
		// A vector of all wavespawns that have been instantiated so far.
		std::vector<wavespawn> wavespawns;
		// An unordered set of all of the icons that are part of the wave so far.
		std::unordered_set<std::string> class_icons;
		// How much pressure is currently being placed on the theoretical players of the mission.
		// More pressure means the players are having a harder time.
		// We're assuming our players are disposing of the robot baddies over time,
		// so this pressure value should decay over time if left unattended.
		// If there's too much pressure, we should ease up on the players.
		// If there's too little pressure, we should go harder on the players.
		float pressure = 0.0f;

		// The more currency the players have, the more damage they'll be able to deal.
		// The more damage the players can deal, the quicker they'll be able to dispose of the enemies.
		// The number of players also affects how quickly the robots can be dealt with.
		// Let's have the pressure decay based on this quantity of currency multiplied by the number of players.
		// This decay rate is measured per second.
		// Since this decay rate essentially calculates how powerful and capable the RED team is,
		// it is also used to calculate how powerful the robots should be.
		float pressure_decay_rate = (current_currency * currency_pressure_multiplier + base_pressure_decay_rate) * players * pressure_decay_rate_multiplier;

		std::cout << "The pressure decay rate is " << pressure_decay_rate << '.' << std::endl;

		// Count the number of active wavespawns.
		// We'll use this to scale pressure based on how many concurrent wavespawns are occurring.
		int active_wavespawns = 0;

		/*
		int tanks = 0;
		while (rand_chance(tank_chance) && tanks < 137)
		{
			++tanks;
			if (tank_chance >= 1.0f)
			{
				break;
			}
		}
		if (tanks != 0)
		{
			class_icons.emplace("tank");

			float absolute_end_time = max_time * 0.8f;
			float first_tank_time = rand_float(0, absolute_end_time * 0.8f);
			float remaining_time = absolute_end_time - first_tank_time;
			float time_between_spawns = remaining_time / (tanks + 1);
			float speed = rand_float(10, 100);
			int health = static_cast<int>(ceil((rand_float(5, 12) * 100000 / speed) / 1000) * 1000);
			
			wavespawn ws;
			std::stringstream wsname;
			wsname << "\"wave" << current_wave << '_' << wavespawns.size() + 1 << '\"';
			ws.name = wsname.str();
			ws.total_count = tanks;
			ws.wait_before_starting = first_tank_time;
			ws.wait_between_spawns = time_between_spawns;
			ws.time_until_next_spawn = ws.wait_before_starting;
			ws.spawns_remaining = ws.total_count;
			ws.effective_pressure = static_cast<float>(health);
			ws.time_to_kill = ws.effective_pressure / pressure_decay_rate;
			ws.type_of_spawned = wavespawn::type::tank;
			ws.tnk.speed = speed;
			ws.tnk.health = health;
			wavespawns.emplace_back(ws);
		}

		std::cout << "Generated " << tanks << " tank(s)." << std::endl;
		*/

		// This loop generates all of the WaveSpawns.
		while (t < max_time && wavespawns.size() < max_wavespawns && class_icons.size() < max_icons)
		{
			// The rate at which the pressure is decaying when all active wavespawns are taken into consideration.
			/*
			float effective_pressure_decay_rate;

			calculate_effective_pressure_decay_rate(pressure_decay_rate, effective_pressure_decay_rate, wavespawns, t);

			while (effective_pressure_decay_rate > 0 && t < max_time && wavespawns.size() < max_wavespawns && class_icons.size() < max_icons)
			{
				std::cout << "Effective pressure decay rate > 0." << std::endl;
				*/

			std::cout << "Generating new wavespawn at t = " << t << '.' << std::endl;

			if (rand_chance(tank_chance))
			{
				// Generate a new tank WaveSpawn.

				class_icons.emplace("tank");

				float speed = rand_float(10, 100);
				int health = rand_int(1, 100) * 1000;

				float speed_pressure = ((speed - 10.0f) * 0.1f) + 1.0f;

				// How long it should take to kill the theoretical bot.
				float time_to_kill;

				// Cache the recipricol of the pressure decay rate.
				const float recip_pressure_decay_rate = 1 / pressure_decay_rate;

				float effective_pressure;
				float wait_between_spawns;
				int max_count = 0;

				while (max_count == 0)
				{
					effective_pressure = health * speed_pressure;
					time_to_kill = effective_pressure * recip_pressure_decay_rate;
					wait_between_spawns = time_to_kill * rand_float(1.0f, 5.0f);
					max_count = static_cast<int>(floor((max_time - t) / wait_between_spawns));

					// If the max count is too low, it means the bot may be too strong.
					if (max_count == 0 && health > 1000)
					{
						health = static_cast<int>(health * 0.9f);
					}
					else
					{
						max_count = 1;
					}
				}

				// Round the tank health to the nearest 1000.
				health = static_cast<int>(std::ceil(static_cast<float>(health) / 1000) * 1000);

				// Add the tank WaveSpawn to the wavespawns vector.
				wavespawn ws;
				std::stringstream wsname;
				wsname << "\"wave" << current_wave << '_' << wavespawns.size() + 1 << '\"';
				ws.name = wsname.str();
				ws.total_count = rand_int(1, max_count + 1);
				ws.wait_before_starting = static_cast<float>(t);
				ws.wait_between_spawns = wait_between_spawns;
				ws.spawns_remaining = ws.total_count - 1;
				ws.effective_pressure = effective_pressure;
				ws.time_to_kill = time_to_kill;
				ws.type_of_spawned = wavespawn::type::tank;
				ws.tnk.speed = speed;
				ws.tnk.health = health;
				ws.time_until_next_spawn = ws.wait_between_spawns;
				ws.time_to_kill_expires = ws.time_to_kill;
				wavespawns.emplace_back(ws);

				++active_wavespawns;
				pressure += ws.effective_pressure; // *active_wavespawns;

				std::cout << "Added tank to wavespawns." << std::endl;
			}
			else
			{
				// Generate a new TFBot WaveSpawn.

				botgen.set_pressure_decay_rate(pressure_decay_rate);
				tfbot_meta bot_meta = botgen.generate_bot();
				tfbot& bot = bot_meta.get_bot();

				// Calculate WaveSpawn data for the TFBot.
				// The following loop makes sure the TFBot doesn't have too much health to handle.

				std::cout << "Entering TFBot TotalCount calculation loop..." << std::endl;

				// How long it should take to kill the theoretical bot.
				float time_to_kill;
				//float effective_time_to_kill;

				// Cache the recipricol of the pressure decay rate.
				const float recip_pressure_decay_rate = 1 / pressure_decay_rate;

				/*
				// Will be 0 if there's no pressure.
				float pressure_compensation = pressure * recip_pressure_decay_rate;
				if (pressure_compensation < 1.0f)
				{
					pressure_compensation = 1.0f;
				}
				else
				{
					pressure_compensation = ((pressure_compensation - 1.0f) * 0.2f) + 1.0f;
					std::cout << "Pressure is nonzero (" << pressure << "). Pressure compensation: " << pressure_compensation << '.' << std::endl;
					std::getchar();
				}
				*/

				const float time_left = std::min(max_time - t, max_tfbot_wavespawn_time);

				float effective_pressure;
				float wait_between_spawns;
				int max_count = 0;

				while (max_count == 0)
				{
					effective_pressure = bot_meta.pressure * bot.health;
					time_to_kill = effective_pressure * recip_pressure_decay_rate;
					wait_between_spawns = time_to_kill * rand_float(1.0f, 5.0f); // * pressure_compensation;
					max_count = static_cast<int>(floor(time_left / wait_between_spawns));

					//effective_time_to_kill = effective_pressure / effective_pressure_decay_rate;

					//bool has_pressure_issue = (pressure_compensation > 0.0f && bot.health > pressure_decay_rate * 100.0f / pressure_compensation);

					// If the max count is too low, it means the bot may be too strong.
					if (max_count == 0 && bot.health > 25)
					{
						//max_count = 0;
						// If the wave isn't almost over, keep dwindling the bot's health down.
						if (max_time - t > 20)
						{
							bot.health = static_cast<int>(bot.health * 0.9f);
						}
						// If the wave is almost over...
						else
						{
							// Just let the bot live at its full strength if it's strong, but not TOO strong.
							max_count = floor(max_time / wait_between_spawns);
							if (max_count != 0)
							{
								max_count = 1;
							}
						}
					}
					else if (wait_between_spawns < 1.0f)
					{
						if (rand_chance(0.9f))
						{
							if (bot_meta.isGiant)
							{
								bot.health *= 2;
							}
							else if (!bot_meta.permaSmall)
							{
								botgen.make_bot_into_giant(bot_meta);
							}
							else
							{
								bot.health *= 2;
							}
							max_count = 0;
						}
					}
					else if (bot_meta.isBoss || bot.health <= 25)
					{
						max_count = 1;
					}
				}

				//max_count = ceil(static_cast<float>(max_count) * 0.7f);

				std::cout << "TotalCount calculation complete." << std::endl;
				std::cout << "Post-TotalCount loop bot health: " << bot.health << std::endl;
				std::cout << "The bot's effective pressure (with health): " << effective_pressure << std::endl;
				std::cout << "TotalCount: " << max_count << std::endl;
				std::cout << "WaitBetweenSpawns: " << wait_between_spawns << std::endl;

				class_icons.emplace(bot.class_icon);

				std::cout << "Total class icons so far: " << class_icons.size() << '.' << std::endl;

				bot.character_attributes.emplace_back("move speed bonus", bot_meta.move_speed_bonus);

				wavespawn ws;
				std::stringstream wsname;
				wsname << "\"wave" << current_wave << '_' << wavespawns.size() + 1 << '\"';
				ws.name = wsname.str();
				ws.total_count = rand_int(1, max_count + 1);
				ws.max_active = std::min(22, ws.total_count);
				ws.wait_before_starting = static_cast<float>(t);
				ws.wait_between_spawns = wait_between_spawns;
				ws.bot = bot;
				ws.effective_pressure = effective_pressure;
				ws.time_to_kill = time_to_kill;
				ws.spawns_remaining = ws.total_count - 1;
				ws.type_of_spawned = wavespawn::type::tfbot;
				ws.time_until_next_spawn = ws.wait_between_spawns;
				ws.time_to_kill_expires = ws.time_to_kill;
				wavespawns.emplace_back(ws);

				++active_wavespawns;
				pressure += ws.effective_pressure; // *active_wavespawns;

				std::cout << "Added bot to wavespawns." << std::endl;
			}
			/*
			// Recalculate the effective pressure decay rate.
			calculate_effective_pressure_decay_rate(pressure_decay_rate, effective_pressure_decay_rate, wavespawns, t);
		}
		*/

			// Time to do any final work before the next loop iteration (if there is one).

			std::cout << "wave_generator pressure: " << pressure << std::endl;

			// An empty second is a second that occurs without a single new WaveSpawn.
			// To prevent boredom, this amount of time is capped.
			//int empty_seconds = 0;
			while (pressure > 0.0f
				//&& empty_seconds < 3
				)
			{
				// Increment time.
				++t;
				//++empty_seconds;

				float pressure_increase = 0.0f;
				// Iterate through the wavespawns to update the pressure.
				for (wavespawn& ws : wavespawns)
				{
					// Time To Kill Expires Greater Than 0
					bool ttkegtz = false;
					if (ws.time_to_kill_expires > 0.0f)
					{
						ttkegtz = true;
					}
					ws.time_to_kill_expires -= 1.0f;
					if (ttkegtz && ws.time_to_kill_expires <= 0.0f)
					{
						--active_wavespawns;
						ttkegtz = false;
					}

					ws.time_until_next_spawn -= 1.0f;

					while (ws.spawns_remaining != 0 && ws.time_until_next_spawn <= 0.0f)
					{
						/*
						if (ws.spawns_remaining == ws.total_count)
						{
							++active_wavespawns;
						}
						if (ws.spawns_remaining == 0)
						{
							--active_wavespawns;
						}
						else
						{
						*/
						pressure_increase += ws.effective_pressure;
						ws.time_until_next_spawn += ws.wait_between_spawns;

						if (!ttkegtz)
						{
							++active_wavespawns;
						}
						ws.time_to_kill_expires = ws.time_to_kill;
						ttkegtz = true;
						//}
						ws.spawns_remaining -= 1;

						// Since something just spawned, reset the empty seconds.
						//empty_seconds = 0;
						//std::cout << "Empty seconds reset." << std::endl;
					}
				}

				// Modify the pressure.
				pressure += pressure_increase; // *active_wavespawns;
				// Since time has passed, reduce the pressure.
				pressure -= pressure_decay_rate * pressure_decay_rate_multiplier_in_time / (active_wavespawns + 1);
			}

			//++t;

			std::cout << "t = " << t << std::endl;
		}

		std::cout << "Wave generation complete. Writing to popfile..." << std::endl;

		current_currency += currency_per_wave;
		int currency_per_wavespawn = currency_per_wave / wavespawns.size();

		// Write the WaveSpawns.
		for (const wavespawn& ws : wavespawns)
		{
			block_start("WaveSpawn");
			write("Name", ws.name);
			write("TotalCount", ws.total_count);
			write("WaitBeforeStarting", ws.wait_before_starting);
			write("WaitBetweenSpawns", ws.wait_between_spawns);
			write("TotalCurrency", currency_per_wavespawn);
			if (ws.type_of_spawned == wavespawn::type::tfbot)
			{
				write("SpawnCount", ws.spawn_count);
				write("MaxActive", ws.max_active);
				write("Where", "spawnbot");
				write_blank();
				block_start("TFBot");
				write("Class", player_class_to_string(ws.bot.cl));
				write("Name \"" + ws.bot.name + '\"');
				write("ClassIcon", ws.bot.class_icon);
				write("Health", ws.bot.health);
				if (ws.bot.scale > 0.0f)
				{
					write("Scale", ws.bot.scale);
				}
				write("Skill", ws.bot.skill);

				if (ws.bot.weapon_restrictions != "")
				{
					write("WeaponRestrictions", ws.bot.weapon_restrictions);
				}
				if (ws.bot.max_vision_range > 0.0f)
				{
					write("MaxVisionRange", ws.bot.max_vision_range);
				}
				if (ws.bot.cl == player_class::engineer)
				{
					write("TeleportWhere", "spawnbot");
				}

				// Write all Attributes.
				int attributes_count = ws.bot.attributes.size();
				for (int i = 0; i < attributes_count; ++i)
				{
					write("Attributes", ws.bot.attributes.at(i));
				}

				if (ws.bot.auto_jump_min != 0.0f)
				{
					write("AutoJumpMin", ws.bot.auto_jump_min);
					write("AutoJumpMax", ws.bot.auto_jump_max);
				}
				
				// Write Items.
				int items_count = ws.bot.items.size();
				for (int i = 0; i < items_count; ++i)
				{
					write("Item", '\"' + ws.bot.items.at(i) + '\"');
				}

				// Write CharacterAttributes.
				int character_attributes_count = ws.bot.character_attributes.size();
				if (character_attributes_count != 0)
				{
					block_start("CharacterAttributes");
					for (int i = 0; i < character_attributes_count; ++i)
					{
						write('\"' + ws.bot.character_attributes.at(i).first + '\"', ws.bot.character_attributes.at(i).second);
					}
					block_end(); // CharacterAttributes
				}

				block_end(); // TFBot
			}
			else if (ws.type_of_spawned == wavespawn::type::tank)
			{
				write_blank();
				block_start("FirstSpawnOutput");
				write("Target", "boss_spawn_relay");
				write("Action", "Trigger");
				block_end(); // FirstSpawnOutput
				write_blank();
				block_start("Tank");
				write("Health", ws.tnk.health);
				write("Speed", ws.tnk.speed);
				write("Name", "\"tankboss\"");
				write("StartingPathTrackNode", "\"boss_path_a1\"");
				block_start("OnKilledOutput");
				write("Target", "boss_dead_relay");
				write("Action", "Trigger");
				block_end(); // OnKilledOutput
				block_start("OnBombDroppedOutput");
				write("Target", "boss_deploy_relay");
				write("Action", "Trigger");
				block_end(); // OnBombDroppedOutput
				block_end(); // Tank
			}

			block_end(); // WaveSpawn
		}
		block_end(); // Wave
	}

	block_end(); // WaveSchedule
	popfile.close();

	std::cout << "Write complete." << std::endl;
}

void wave_generator::write_indents()
{
	for (int i = 0; i < indent; ++i)
	{
		popfile << '\t';
	}
}

void wave_generator::write(const std::string& str)
{
	write_indents();
	popfile << str << '\n';
}

void wave_generator::write(const std::string& str, int number)
{
	write_indents();
	popfile << str << ' ' << number << '\n';
}

void wave_generator::write(const std::string& str, float number)
{
	write_indents();
	popfile << str << ' ' << number << '\n';
}

void wave_generator::write(const std::string& str1, const std::string& str2)
{
	write_indents();
	popfile << str1 << ' ' << str2 << '\n';
}

void wave_generator::write_blank()
{
	popfile << '\n';
}

void wave_generator::block_start(const std::string& str)
{
	write(str);
	write("{");
	++indent;
}

void wave_generator::block_end()
{
	--indent;
	write("}");
}

void wave_generator::calculate_effective_pressure_decay_rate
(const float& pressure_decay_rate, float& effective_pressure_decay_rate, const std::vector<wavespawn>& wavespawns, const int& t)
{
	effective_pressure_decay_rate = pressure_decay_rate;

	std::cout << wavespawns.size() << " wavespawns generated thus far." << std::endl;

	for (const wavespawn& ws : wavespawns)
	{
		/*
		if (ws.spawns_remaining != 0
			//&& t >= ws.wait_before_starting
			)
		*/

		float time_of_last_spawn = ws.wait_before_starting + ws.wait_between_spawns * (ws.total_count - 1);
		float time_of_last_dead = time_of_last_spawn + ws.time_to_kill;

		if (t >= ws.wait_before_starting && t <= time_of_last_dead)
		{
			//std::cout << "Wavespawn is active!" << std::endl;

			float wavespawn_pressure_per_second = ws.effective_pressure / ws.wait_between_spawns;

			/*
			std::cout << "Wavespawn's effective pressure: " << ws.effective_pressure << std::endl;
			std::cout << "Wavespawn's wait between spawns: " << ws.wait_between_spawns << std::endl;
			std::cout << "Wavespawn's pressure per second: " << wavespawn_pressure_per_second << std::endl;
			*/

			effective_pressure_decay_rate -= wavespawn_pressure_per_second;
		}
	}

	std::cout << "Effective pressure decay rate: " << effective_pressure_decay_rate << std::endl;
}