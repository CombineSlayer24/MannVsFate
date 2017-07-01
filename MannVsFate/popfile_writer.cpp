#include "popfile_writer.h"

void popfile_writer::popfile_open(const std::string& filename)
{
	popfile.open(filename);
	if (!popfile)
	{
		const std::string exstr = "popfile_writer::popfile_open exception: Couldn't write to file \"" + filename + "\".";
		throw std::exception(exstr.c_str());
	}
}

void popfile_writer::popfile_close()
{
	popfile.close();
}

void popfile_writer::popfile_copy_write(const std::string& filename)
{
	std::ifstream in(filename);//, std::ios_base::binary);
	popfile << in.rdbuf();
}

void popfile_writer::write_indents()
{
	for (int i = 0; i < indent; ++i)
	{
		popfile << '\t';
	}
}

void popfile_writer::write(const std::string& str)
{
	write_indents();
	popfile << str << '\n';
}

void popfile_writer::write(const std::string& str, int number)
{
	write_indents();
	popfile << str << ' ' << number << '\n';
}

void popfile_writer::write(const std::string& str, float number)
{
	write_indents();
	popfile << str << ' ' << number << '\n';
}

void popfile_writer::write(const std::string& str1, const std::string& str2)
{
	write_indents();
	popfile << str1 << ' ' << str2 << '\n';
}

void popfile_writer::write_blank()
{
	popfile << '\n';
}

void popfile_writer::block_start(const std::string& str)
{
	write(str);
	write("{");
	++indent;
}

void popfile_writer::block_end()
{
	--indent;
	write("}");
}

void popfile_writer::write_popfile_header(const std::string& generator_version, int argc, char** argv)
{
	// Write a bit of nice info at the top of the mission file.
	write("// This mission file was randomly generated by Mann Vs Fate version " + generator_version + '.');
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
	//write("#base robot_giant.pop"); // Needed for the Sentry Buster template.
	//write_blank();
}

void popfile_writer::write_wave_divider(int wave_number)
{
	write_indents();
	popfile << "// WAVE " << wave_number <<
		" ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n";
}

void popfile_writer::write_wave_header(const std::string& wave_start_relay, const std::string& wave_finished_relay)
{
	block_start("Wave");
	block_start("StartWaveOutput");
	write("Target", wave_start_relay);
	write("Action", "Trigger");
	block_end(); // StartWaveOutput
	block_start("DoneOutput");
	write("Target", wave_finished_relay);
	write("Action", "Trigger");
	block_end(); // DoneOutPut
}

void popfile_writer::write_tfbot(const tfbot& bot, const std::vector<std::string>& spawnbots)
{
	block_start("TFBot");

	write("Class", player_class_to_string(bot.cl));
	write("Name \"" + bot.name + '\"');
	write("ClassIcon", bot.class_icon);
	write("Health", bot.health);
	if (bot.scale > 0.0f)
	{
		write("Scale", bot.scale);
	}
	write("Skill", bot.skill);

	if (bot.weapon_restrictions != "")
	{
		write("WeaponRestrictions", bot.weapon_restrictions);
	}
	if (bot.max_vision_range > 0.0f)
	{
		write("MaxVisionRange", bot.max_vision_range);
	}
	if (bot.cl == player_class::engineer)
	{
		for (const std::string& spawnbot : spawnbots)
		{
			write("TeleportWhere", spawnbot);
		}
	}

	// Write all Attributes.
	int attributes_count = bot.attributes.size();
	for (int i = 0; i < attributes_count; ++i)
	{
		write("Attributes", bot.attributes.at(i));
	}

	if (bot.auto_jump_min != 0.0f)
	{
		write("AutoJumpMin", bot.auto_jump_min);
		write("AutoJumpMax", bot.auto_jump_max);
	}

	// Write Items.
	int items_count = bot.items.size();
	for (int i = 0; i < items_count; ++i)
	{
		write("Item", '\"' + bot.items.at(i) + '\"');
	}

	// Write CharacterAttributes.
	int character_attributes_count = bot.character_attributes.size();
	if (character_attributes_count != 0)
	{
		block_start("CharacterAttributes");
		for (int i = 0; i < character_attributes_count; ++i)
		{
			write('\"' + bot.character_attributes.at(i).first + '\"', bot.character_attributes.at(i).second);
		}
		block_end(); // CharacterAttributes
	}

	block_end(); // TFBot
}

void popfile_writer::write_wavespawn(const wavespawn& ws, const std::vector<std::string>& spawnbots)
{
	block_start("WaveSpawn");

	write("Name", ws.name);

	if (ws.first_spawn_warning_sound != "")
	{
		write("FirstSpawnWarningSound", '\"' + ws.first_spawn_warning_sound + '\"');
	}

	write("TotalCount", ws.total_count);
	write("WaitBeforeStarting", ws.wait_before_starting);
	write("WaitBetweenSpawns", ws.wait_between_spawns);
	write("TotalCurrency", ws.total_currency);
	if (ws.support == wavespawn::support_type::unlimited)
	{
		write("Support", 1);
	}
	else if (ws.support == wavespawn::support_type::limited)
	{
		write("Support", "Limited");
	}
	if (ws.type_of_spawned == wavespawn::type::tfbot)
	{
		write("SpawnCount", ws.spawn_count);
		write("MaxActive", ws.max_active);
		write("Where", ws.location);
		write_blank();
		write_tfbot(ws.bot, spawnbots);
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
		write("StartingPathTrackNode", ws.location);
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

void popfile_writer::write_mission(const mission& mis, const std::vector<std::string>& spawnbots)
{
	block_start("Mission");
	write("Objective", mis.objective);
	write("InitialCooldown", mis.initial_cooldown);
	write("Where", mis.location);
	write("BeginAtWave", mis.begin_at_wave);
	write("RunForThisManyWaves", mis.run_for_this_many_waves);
	write("CooldownTime", mis.cooldown_time);
	write_tfbot(mis.bot, spawnbots);

	/*
	// The default Sentry Buster.
	block_start("TFBot");
	write("Template", "T_TFBot_SentryBuster");
	block_end(); // TFBot
	*/

	block_end(); // Mission
}