// An extension of the tfbot class.

#ifndef TFBOT_META_H
#define TFBOT_META_H

#include "tfbot.h"

class tfbot_meta
{
public:
	// Access the bot.
	tfbot& get_bot();

	// Make this bot into a giant.
	void make_giant();

	// Update the bot's class icon based on the base class icon and the giants and crits statuses.
	void update_class_icon();

	// Set the bot's base class icon (excluding crits and giant). Also updates the wrapped bot's icon accordingly.
	void set_base_class_icon(const std::string& new_base_class_icon);

	// Get the base class icon of the bot, excluding crits and giant status.
	std::string get_base_class_icon() const;

	// Returns the effective pressure of the bot.
	float calculate_effective_pressure() const;

	// Returns the raw pressure of the bot, but muted.
	// This places more emphasis on the bot's health.
	float calculate_muted_pressure() const;

	// Calculates the effective pressure of the bot using the muted pressure.
	float calculate_muted_effective_pressure() const;

	// Returns how long it should take to kill the theoretical bot based on the current pressure decay rate.
	float calculate_time_to_kill(float recip_pressure_decay_rate) const;

	// Calculate the time to kill with more emphasis on health and less emphasis on pressure.
	// The pressure is muted in the calculation.
	float calculate_muted_time_to_kill(float recip_pressure_decay_rate) const;

	// This is the multiplier for calculating the bot's threat level by multiplying its health.
	// The multiplier is adjusted based on a variety of factors.
	// A higher multiplier means the bot is harder to deal with.
	float pressure = 1.0f;
	// The move speed bonus of the bot.
	float move_speed_bonus = 1.0f;
	// The damage bonus of the bot.
	float damage_bonus = 1.0f;
	// Whether the bot is a giant.
	bool is_giant = false;
	// Whether the bot is a boss (i.e. unusually strong giant).
	bool is_boss = false;
	// Whether the bot is a doombot (i.e. enormous giant).
	bool is_doom = false;
	// Whether the bot cannot become a boss.
	bool perma_small = false;
	// Whether the bot has the AlwaysCrit attribute.
	bool is_always_crit = false;
	// Whether the bot has the AlwaysFireWeapon attribute.
	bool is_always_fire_weapon = false;

private:
	// The bot being wrapped.
	tfbot bot;
	// The base class icon of the bot, not including the giant or crits components.
	std::string base_class_icon = "";
};

#endif // TFBOT_META_H