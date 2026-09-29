#pragma once

class PlayerManager;
class TrackCollectionManager;

/// Registers the [Export] and per-deck export trigger ControlObjects and
/// connects them to the exporter implementations.
///
/// @param pTrackCollectionManager used by the Engine exporter (upstream job).
/// @param pPlayerManager used to resolve which track is loaded on a deck.
///        Passing nullptr registers the controls but per-deck export becomes a
///        logged no-op.
void registerExportControls(PlayerManager* pPlayerManager,
        TrackCollectionManager* pTrackCollectionManager);
