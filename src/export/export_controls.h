#pragma once

class PlayerManager;

/// Registers the [Export] and per-deck export trigger ControlObjects and
/// connects them to the exporter implementations.
///
/// @param pPlayerManager used to resolve which track is loaded on a deck.
///        Passing nullptr registers the controls but per-deck export becomes a
///        logged no-op.
void registerExportControls(PlayerManager* pPlayerManager);
