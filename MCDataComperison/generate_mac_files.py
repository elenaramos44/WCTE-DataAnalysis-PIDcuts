import os
import random
from pathlib import Path

# === USER DEFINED PARAMETERS ===
NEV = 1000                 # Number of events per file
NUM_FILES = 3              # Number of files to generate
PARTICLE = "pi-"           # Geant4 particle name: pi+, mu-, e+, etc.
MOMENTUM = 350.0           # Momentum in MeV/c
TEMPLATE_FILE = "template.mac"  # Template file name (must be in current directory)

# === PARTICLE MASS TABLE (MeV/c^2) ===
MASS_DICT = {
    "e+": 0.511,
    "e-": 0.511,
    "mu+": 105.66,
    "mu-": 105.66,
    "pi+": 139.57,
    "pi-": 139.57,
    "kaon+": 493.68,
    "kaon-": 493.68,
    "proton": 938.27,
    "neutron": 939.57,
}

# === VALIDATION ===
if PARTICLE not in MASS_DICT:
    raise ValueError(f"Unknown particle type '{PARTICLE}'. Please check MASS_DICT.")

# === CALCULATE KINETIC ENERGY ===
mass = MASS_DICT[PARTICLE]
total_energy = (MOMENTUM ** 2 + mass ** 2) ** 0.5
kinetic_energy = total_energy - mass

# === SETUP OUTPUT FOLDER ===
base_name = f"{PARTICLE}_{int(MOMENTUM)}MeVc_{int(NEV)}evts"
output_dir = Path.cwd() / base_name
output_dir.mkdir(exist_ok=True)

# === LOAD TEMPLATE ===
with open(TEMPLATE_FILE, "r") as f:
    template = f.read()

# === GENERATE MAC FILES ===
for i in range(1, NUM_FILES + 1):
    seed = random.randint(100000, 999999)
    filename_prefix = f"{base_name}_{i}"
    outfile_name = f"{filename_prefix}.root"
    mac_filename = f"{filename_prefix}.mac"

    # Replace template variables
    content = (
        template
        .replace("$NEV", str(NEV))
        .replace("$PARTICLE", PARTICLE)
        .replace("$ENERGY", f"{kinetic_energy:.2f}")
        .replace("$SEED", str(seed))
        .replace("$OUTFILE", outfile_name)
    )

    # Write to .mac file
    with open(output_dir / mac_filename, "w") as out_mac:
        out_mac.write(content)

print(f"Generated {NUM_FILES} .mac files in folder: {output_dir}")
