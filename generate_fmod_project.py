#!/usr/bin/env python3
"""
Generate FMOD Studio project for Ferrenzo LMH (VRC PT 2024 Ferrenzo CSP)
Creates a complete .fspro project with all events, parameters, buses, VCAs, and banks
matching the GUIDs.txt from the original mod.

Run: python generate_fmod_project.py
Output: Ferrenzo_LMH.fspro + Project/ folder
"""

import xml.etree.ElementTree as ET
import uuid
import os
import shutil
from pathlib import Path

# ──────────────────────────────────────────────────────────────────────────────
# Fixed GUIDs (from original GUIDs.txt) — keep these stable for bank compatibility
# ──────────────────────────────────────────────────────────────────────────────

GUID = {
    # Banks
    "bank_base":           "711f88d3-a06c-4b7a-a176-428d08a5b41f",  # carname
    "bank_common":         "47dcf37a-6a8d-4ef6-9f82-2e0398fa69f9",  # common
    "bank_csp":            "baeb44f0-b55d-4065-8440-9e2df0177e34",  # carname_csp

    # VCAs
    "vca_engine_int":      "94f04703-a92c-49cf-a097-734f3e79ec08",
    "vca_engine_ext":      "ff157bb9-ecc1-49da-ad82-5e42fecb2ac9",
    "vca_skid":            "f8b9d9e5-bd29-4a82-a1a0-66e8b9059ccb",
    "vca_surfaces":        "45401f11-f5bd-429f-afb6-71c819bb20db",
    "vca_transmission":    "a61eba16-8f14-4a39-8ffd-c573cfdc0dbd",
    "vca_wind":            "185673e8-d3ed-434c-acd4-61c819bb209c",

    # Bus Groups (Grp)
    "grp_engine_int":      "{grp_engine_int}",
    "grp_engine_ext":      "{grp_engine_ext}",
    "grp_transmission":    "{grp_transmission}",
    "grp_skid_int":        "{grp_skid_int}",
    "grp_skid_ext":        "{grp_skid_ext}",
    "grp_wind_closed":     "{grp_wind_closed}",
    "grp_wind_open":       "{grp_wind_open}",
    "grp_surfaces":        "{grp_surfaces}",
    "grp_turbo":           "{grp_turbo}",
    "grp_backfire_int":    "{grp_backfire_int}",
    "grp_backfire_ext":    "{grp_backfire_ext}",
    "grp_gear_int":        "{grp_gear_int}",
    "grp_gear_ext":        "{grp_gear_ext}",
    "grp_limiter":         "{grp_limiter}",
    "grp_tractioncontrol_int": "{grp_tractioncontrol_int}",
    "grp_tractioncontrol_ext": "{grp_tractioncontrol_ext}",
    "grp_wheel_closed":    "{grp_wheel_closed}",
    "grp_wheel_open":      "{grp_wheel_open}",
    "grp_door":            "{grp_door}",
    "grp_horn":            "{grp_horn}",
    "grp_collisions":      "{grp_collisions}",
    "grp_bodywork":        "{grp_bodywork}",
    "grp_turbo":           "{grp_turbo}", 
}

# Generate deterministic GUIDs for buses from their names
def bus_guid(name: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_DNS, f"bus.{name}.carname"))

# Override bus GUIDs with deterministic ones
for k in list(GUID.keys()):
    if k.startswith("grp_"):
        GUID[k] = bus_guid(k)

# ──────────────────────────────────────────────────────────────────────────────
# Parameter Definitions (name, min, max, default, type)
# ──────────────────────────────────────────────────────────────────────────────

PARAMETERS = [
    # Global game parameters
    ("RPM", 0, 12000, 0, "game"),
    ("Load", 0, 1, 0, "game"),
    ("Throttle", 0, 1, 0, "game"),
    ("Speed", 0, 400, 0, "game"),
    ("Gear", -1, 7, 0, "game"),
    ("TurboBoost", 0, 2.5, 0, "game"),
    ("SlipFL", 0, 2, 0, "game"),
    ("SlipFR", 0, 2, 0, "game"),
    ("SlipRL", 0, 2, 0, "game"),
    ("SlipRR", 0, 2, 0, "game"),
    ("SlipAngleFL", -40, 40, 0, "game"),
    ("SlipAngleFR", -40, 40, 0, "game"),
    ("SlipAngleRL", -40, 40, 0, "game"),
    ("SlipAngleRR", -40, 40, 0, "game"),
    ("SuspensionTravelFL", -0.3, 0.3, 0, "game"),
    ("SuspensionTravelFR", -0.3, 0.3, 0, "game"),
    ("SuspensionTravelRL", -0.3, 0.3, 0, "game"),
    ("SuspensionTravelRR", -0.3, 0.3, 0, "game"),
    ("SuspensionVelocityFL", -10, 10, 0, "game"),
    ("SuspensionVelocityFR", -10, 10, 0, "game"),
    ("SuspensionVelocityRL", -10, 10, 0, "game"),
    ("SuspensionVelocityRR", -10, 10, 0, "game"),
    ("BrakePressure", 0, 1, 0, "game"),
    ("BrakeTempFL", 0, 1200, 0, "game"),
    ("BrakeTempFR", 0, 1200, 0, "game"),
    ("BrakeTempRL", 0, 1200, 0, "game"),
    ("BrakeTempRR", 0, 1200, 0, "game"),
    ("SurfaceFL", 0, 10, 0, "game"),
    ("SurfaceFR", 0, 10, 0, "game"),
    ("SurfaceRL", 0, 10, 0, "game"),
    ("SurfaceRR", 0, 10, 0, "game"),
    ("RainIntensity", 0, 1, 0, "game"),
    ("IsInterior", 0, 1, 0, "game"),
    ("IsAI", 0, 1, 0, "game"),
    # CSP hybrid/electrical
    ("KERSInput", 0, 1, 0, "game"),
    ("KERSCharge", 0, 1, 0, "game"),
    ("KERSDeploy", 0, 1, 0, "game"),
    ("MGUHMode", 0, 2, 0, "game"),
    ("IgnitionState", 0, 3, 0, "game"),
    ("StarterEngaged", 0, 1, 0, "game"),
    ("ImpactImpulse", 0, 50000, 0, "game"),
    ("DoorState", 0, 1, 0, "game"),
    ("HornPressed", 0, 1, 0, "game"),
    ("PitLimiter", 0, 1, 0, "game"),
    ("ElectricalLoad", 0, 1, 0, "game"),
    ("AlertType", 0, 4, 0, "game"),
]

# ──────────────────────────────────────────────────────────────────────────────
# Event Definitions
# bank: "base" or "csp"
# ──────────────────────────────────────────────────────────────────────────────

EVENTS_BASE = [
    # name, kind, parameters[], bus_target
    ("engine_int", "timeline", ["RPM", "Load", "Throttle", "Gear", "TurboBoost", "IsInterior"], "grp_engine_int"),
    ("engine_ext", "timeline", ["RPM", "Load", "Throttle", "Gear", "TurboBoost", "Speed", "IsInterior"], "grp_engine_ext"),
    ("limiter", "oneshot", ["RPM", "IsInterior"], "grp_limiter"),
    ("transmission", "timeline", ["RPM", "Gear", "Speed", "Load", "IsInterior"], "grp_transmission"),
    ("transmission_ext", "timeline", ["RPM", "Gear", "Speed", "Load", "IsInterior"], "grp_transmission"),
    ("gear_int", "oneshot", ["Gear", "Clutch", "Speed", "IsInterior"], "grp_gear_int"),
    ("gear_ext", "oneshot", ["Gear", "Clutch", "Speed"], "grp_gear_ext"),
    ("gear_grind", "oneshot", ["Gear", "Clutch", "RPM", "IsInterior"], "grp_gear_int"),
    ("turbo", "timeline", ["TurboBoost", "RPM", "Throttle", "IsInterior"], "grp_turbo"),
    ("backfire_int", "scatterer", ["RPM", "Throttle", "TurboBoost", "IsInterior"], "grp_backfire_int"),
    ("backfire_ext", "scatterer", ["RPM", "Throttle", "TurboBoost", "Speed"], "grp_backfire_ext"),
    ("skid_int", "scatterer", ["SlipAngleFL", "SlipAngleFR", "SlipAngleRL", "SlipAngleRR",
                               "SurfaceFL", "SurfaceFR", "SurfaceRL", "SurfaceRR",
                               "Load", "RainIntensity"], "grp_skid_int"),
    ("skid_ext", "scatterer", ["SlipRatio", "BrakeTempFL", "BrakeTempFR", "BrakeTempRL", "BrakeTempRR",
                               "SurfaceFL", "SurfaceFR", "SurfaceRL", "SurfaceRR",
                               "RainIntensity", "Speed", "Load"], "grp_skid_ext"),
    ("tractioncontrol_int", "oneshot", ["TCActive", "SlipRatio", "IsInterior"], "grp_tractioncontrol_int"),
    ("tractioncontrol_ext", "oneshot", ["TCActive", "SlipRatio"], "grp_tractioncontrol_ext"),
    ("brakes", "timeline", ["BrakePressure", "BrakeTempFL", "BrakeTempFR", "BrakeTempRL", "BrakeTempRR",
                            "Speed", "IsInterior"], "grp_bodywork"),
    ("bodywork", "oneshot", ["ImpactImpulse", "SuspensionTravelFL", "SuspensionTravelFR",
                             "SuspensionTravelRL", "SuspensionTravelRR", "Speed"], "grp_bodywork"),
    ("wheel", "scatterer", ["Speed", "SurfaceFL", "SurfaceFR", "SurfaceRL", "SurfaceRR",
                            "Load", "RainIntensity"], "grp_wheel_closed"),
    ("wind", "timeline", ["Speed", "IsInterior"], "grp_wind_closed"),
    ("door", "oneshot", ["DoorState", "IsInterior"], "grp_door"),
    ("horn", "oneshot", ["HornPressed"], "grp_horn"),
]

EVENTS_CSP = [
    # Overrides (same name as base, CSP bank takes precedence)
    ("engine_int", "timeline", ["RPM", "Load", "Throttle", "Gear", "TurboBoost", "KERSInput", "IsInterior"], "grp_engine_int"),
    ("engine_ext", "timeline", ["RPM", "Load", "Throttle", "Gear", "TurboBoost", "KERSInput", "Speed", "IsInterior"], "grp_engine_ext"),
    ("limiter", "oneshot", ["RPM", "IsInterior"], "grp_limiter"),
    ("transmission", "timeline", ["RPM", "Gear", "Speed", "Load", "IsInterior"], "grp_transmission"),
    ("gear_int", "oneshot", ["Gear", "Clutch", "Speed", "IsInterior"], "grp_gear_int"),
    ("gear_ext", "oneshot", ["Gear", "Clutch", "Speed"], "grp_gear_ext"),
    ("gear_grind", "oneshot", ["Gear", "Clutch", "RPM", "IsInterior"], "grp_gear_int"),
    ("turbo", "timeline", ["TurboBoost", "RPM", "Throttle", "IsInterior"], "grp_turbo"),
    ("backfire_int", "scatterer", ["RPM", "Throttle", "TurboBoost", "IsInterior"], "grp_backfire_int"),
    ("backfire_ext", "scatterer", ["RPM", "Throttle", "TurboBoost", "Speed"], "grp_backfire_ext"),
    ("skid_int", "scatterer", ["SlipAngleFL", "SlipAngleFR", "SlipAngleRL", "SlipAngleRR",
                               "SurfaceFL", "SurfaceFR", "SurfaceRL", "SurfaceRR",
                               "Load", "RainIntensity"], "grp_skid_int"),
    ("skid_ext", "scatterer", ["SlipRatio", "BrakeTempFL", "BrakeTempFR", "BrakeTempRL", "BrakeTempRR",
                               "SurfaceFL", "SurfaceFR", "SurfaceRL", "SurfaceRR",
                               "RainIntensity", "Speed", "Load"], "grp_skid_ext"),
    ("tractioncontrol_int", "oneshot", ["TCActive", "SlipRatio", "IsInterior"], "grp_tractioncontrol_int"),
    ("tractioncontrol_ext", "oneshot", ["TCActive", "SlipRatio"], "grp_tractioncontrol_ext"),
    ("brakes", "timeline", ["BrakePressure", "BrakeTempFL", "BrakeTempFR", "BrakeTempRL", "BrakeTempRR",
                            "Speed", "IsInterior"], "grp_bodywork"),
    ("bodywork", "oneshot", ["ImpactImpulse", "SuspensionTravelFL", "SuspensionTravelFR",
                             "SuspensionTravelRL", "SuspensionTravelRR", "Speed"], "grp_bodywork"),
    ("wheel", "scatterer", ["Speed", "SurfaceFL", "SurfaceFR", "SurfaceRL", "SurfaceRR",
                            "Load", "RainIntensity"], "grp_wheel_closed"),
    ("wind", "timeline", ["Speed", "IsInterior"], "grp_wind_closed"),
    ("door", "oneshot", ["DoorState", "IsInterior"], "grp_door"),
    ("horn", "oneshot", ["HornPressed"], "grp_horn"),
    # CSP-only events
    ("hybrid_int", "timeline", ["KERSInput", "KERSCharge", "KERSDeploy", "MGUHMode", "IsInterior"], "grp_engine_int"),
    ("hybrid_ext", "timeline", ["KERSInput", "KERSCharge", "KERSDeploy", "MGUHMode", "Speed"], "grp_engine_ext"),
    ("ignition_int", "timeline", ["IgnitionState", "StarterEngaged", "RPM", "IsInterior"], "grp_engine_int"),
    ("ignition_ext", "timeline", ["IgnitionState", "StarterEngaged", "RPM", "Speed"], "grp_engine_ext"),
    ("starter_int", "oneshot", ["StarterEngaged", "RPM", "IsInterior"], "grp_engine_int"),
    ("starter_ext", "oneshot", ["StarterEngaged", "RPM"], "grp_engine_ext"),
    ("chassis_int", "scatterer", ["SuspensionTravelFL", "SuspensionTravelFR", "SuspensionTravelRL", "SuspensionTravelRR",
                                  "SuspensionVelocityFL", "SuspensionVelocityFR", "SuspensionVelocityRL", "SuspensionVelocityRR"], "grp_surfaces"),
    ("chassis_ext", "scatterer", ["SuspensionTravelFL", "SuspensionTravelFR", "SuspensionTravelRL", "SuspensionTravelRR",
                                  "SuspensionVelocityFL", "SuspensionVelocityFR", "SuspensionVelocityRL", "SuspensionVelocityRR"], "grp_surfaces"),
    ("misc_int", "timeline", ["PitLimiter", "ElectricalLoad", "AlertType", "IsInterior"], "grp_engine_int"),
]

# ──────────────────────────────────────────────────────────────────────────────
# XML Generation Helpers
# ──────────────────────────────────────────────────────────────────────────────

NS = "http://www.fmod.org/projects/2020/project"
ET.register_namespace("", NS)

def new_el(tag, **attrib):
    return ET.Element(f"{{{NS}}}{tag}", attrib)

def guid() -> str:
    return "{" + str(uuid.uuid4()).upper() + "}"

def param_guid(name: str) -> str:
    # deterministic per parameter name
    return "{" + str(uuid.uuid5(uuid.NAMESPACE_DNS, f"param.{name}.ferrenzo")).upper() + "}"

# ──────────────────────────────────────────────────────────────────────────────
# Build Project XML
# ──────────────────────────────────────────────────────────────────────────────

def build_project() -> ET.ElementTree:
    root = new_el("project", version="2.02.00", name="Ferrenzo_LMH")

    # ── Meta ──
    meta = new_el("meta")
    meta.append(new_el("createdWith", version="2.02.00"))
    root.append(meta)

    # ── Parameters ──
    params_el = new_el("parameters")
    for name, min_v, max_v, def_v, ptype in PARAMETERS:
        p = new_el("parameter", id=param_guid(name), name=name, type=ptype,
                   minimum=str(min_v), maximum=str(max_v), default=str(def_v))
        params_el.append(p)
    root.append(params_el)

    # ── Buses ──
    buses_el = new_el("buses")
    # Master bus
    master = new_el("bus", id=guid(), name="Master", routing="Master")
    master.append(new_el("volume", value="0"))
    master.append(new_el("pitch", value="0"))
    buses_el.append(master)

    # Group buses
    bus_names = [
        "grp_engine_int", "grp_engine_ext", "grp_transmission", "grp_skid_int", "grp_skid_ext",
        "grp_wind_closed", "grp_wind_open", "grp_surfaces", "grp_turbo", "grp_backfire_int",
        "grp_backfire_ext", "grp_gear_int", "grp_gear_ext", "grp_limiter",
        "grp_tractioncontrol_int", "grp_tractioncontrol_ext", "grp_wheel_closed",
        "grp_wheel_open", "grp_door", "grp_horn", "grp_collisions", "grp_bodywork",
    ]
    for bn in bus_names:
        b = new_el("bus", id=GUID[bn], name=bn, routing="Master")
        b.append(new_el("volume", value="0"))
        b.append(new_el("pitch", value="0"))
        buses_el.append(b)
    root.append(buses_el)

    # ── VCAs ──
    vcas_el = new_el("vcas")
    vca_defs = [
        ("vca_engine_int", "VCA Engine Int"),
        ("vca_engine_ext", "VCA Engine Ext"),
        ("vca_skid", "VCA Skid"),
        ("vca_surfaces", "VCA Surfaces"),
        ("vca_transmission", "VCA Transmission"),
        ("vca_wind", "VCA Wind"),
    ]
    for vca_id, vca_name in vca_defs:
        v = new_el("vca", id=GUID[vca_id], name=vca_name)
        v.append(new_el("volume", value="0"))
        vcas_el.append(v)
    root.append(vcas_el)

    # ── Banks ──
    banks_el = new_el("banks")
    # Base bank
    bb = new_el("bank", id=GUID["bank_base"], name="vrc_pt_2024_ferrenzo",
                type="Master", encoding="FADPCM", sampleRate="48000",
                maxCodecLength="300", maxStreams="32")
    banks_el.append(bb)
    # Common bank (placeholder)
    cb = new_el("bank", id=GUID["bank_common"], name="common",
                type="Master", encoding="FADPCM", sampleRate="48000",
                maxCodecLength="300", maxStreams="32")
    banks_el.append(cb)
    # CSP bank
    csp_b = new_el("bank", id=GUID["bank_csp"], name="vrc_pt_2024_ferrenzo_csp",
                   type="Master", encoding="FADPCM", sampleRate="48000",
                   maxCodecLength="300", maxStreams="32")
    banks_el.append(csp_b)
    root.append(banks_el)

    # ── Events ──
    events_el = new_el("events")

    # Base events
    for ev_name, ev_kind, ev_params, ev_bus in EVENTS_BASE:
        e = new_el("event", id=guid(), name=ev_name, kind=ev_kind,
                   bank=GUID["bank_base"], folder="/cars/vrc_pt_2024_ferrenzo/")
        for pname in ev_params:
            e.append(new_el("parameter", reference=param_guid(pname)))
        e.append(new_el("routing", bus=GUID[ev_bus]))
        events_el.append(e)

    # CSP events (overrides + new)
    for ev_name, ev_kind, ev_params, ev_bus in EVENTS_CSP:
        e = new_el("event", id=guid(), name=ev_name, kind=ev_kind,
                   bank=GUID["bank_csp"], folder="/cars/vrc_pt_2024_ferrenzo_csp/")
        for pname in ev_params:
            e.append(new_el("parameter", reference=param_guid(pname)))
        e.append(new_el("routing", bus=GUID[ev_bus]))
        events_el.append(e)

    root.append(events_el)

    # ── Mixer (bus routing to VCAs) ──
    mixer_el = new_el("mixer")
    # Map group buses to VCAs
    vca_map = {
        "grp_engine_int": "vca_engine_int",
        "grp_engine_ext": "vca_engine_ext",
        "grp_transmission": "vca_transmission",
        "grp_skid_int": "vca_skid",
        "grp_skid_ext": "vca_skid",
        "grp_wind_closed": "vca_wind",
        "grp_wind_open": "vca_wind",
        "grp_surfaces": "vca_surfaces",
        "grp_transmission": "vca_transmission",  # duplicate ok
    }
    for bus_name, vca_name in vca_map.items():
        r = new_el("routing", bus=GUID[bus_name], vca=GUID[vca_name])
        mixer_el.append(r)
    root.append(mixer_el)

    return ET.ElementTree(root)

# ──────────────────────────────────────────────────────────────────────────────
# Write Project Files
# ──────────────────────────────────────────────────────────────────────────────

def write_project(output_dir: Path):
    output_dir = Path(output_dir)
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True)

    # Write .fspro
    tree = build_project()
    fspro_path = output_dir / "Ferrenzo_LMH.fspro"
    tree.write(fspro_path, encoding="utf-8", xml_declaration=True)
    print(f"✅ Written {fspro_path}")

    # Create Project/ folder structure (FMOD expects this)
    proj_dir = output_dir / "Project"
    proj_dir.mkdir()

    # Metadata files (minimal)
    (proj_dir / "Metadata").mkdir()
    (proj_dir / "Events").mkdir()
    (proj_dir / "Banks").mkdir()
    (proj_dir / "Buses").mkdir()
    (proj_dir / "VCAs").mkdir()
    (proj_dir / "Parameters").mkdir()
    (proj_dir / "Mixer").mkdir()

    # Write a simple metadata index so FMOD can open it
    meta_index = new_el("metadataIndex")
    for name, pguid in [(n, param_guid(n)) for n, _, _, _, _ in PARAMETERS]:
        meta_index.append(new_el("parameter", name=name, id=pguid))
    ET.ElementTree(meta_index).write(proj_dir / "Metadata" / "Parameters.xml", encoding="utf-8", xml_declaration=True)

    print(f"📁 Project structure created in {output_dir}")
    print("\nNext steps:")
    print("  1. Open Ferrenzo_LMH.fspro in FMOD Studio 2.02.xx")
    print("  2. Add audio assets to each event (right-click → Add Audio)")
    print("  3. Draw parameter modulation curves per the spec table")
    print("  4. Build → Desktop → copies banks to build/")
    print("  5. Copy build/*.bank + GUIDs.txt to Assetto Corsa content/cars/<car>/sfx/")

if __name__ == "__main__":
    out = Path(__file__).parent / "Ferrenzo_LMH_FMOD_Project"
    write_project(out)
