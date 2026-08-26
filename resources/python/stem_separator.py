#!/usr/bin/env python3
"""
AI Stem Separation for ksAudioStudio
Separates audio tracks into stems using machine learning models.

Currently supports:
- Demucs (requires: pip install demucs)
- Spleeter (requires: pip install spleeter)
- Basic FFT-based separation (fallback)

Usage:
    python stem_separator.py input.wav output_dir --model demucs
    python stem_separator.py input.wav output_dir --model spleeter
    python stem_separator.py input.wav output_dir --method fft
"""

import sys
import os
import subprocess
import json
import tempfile
import struct
from pathlib import Path
from typing import Optional, Dict, List, Any

# Attempt to import available libraries
try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False
    print("WARNING: numpy not available, using basic audio processing")

try:
    import soundfile as sf
    HAS_SOUNDFILE = True
except ImportError:
    HAS_SOUNDFILE = False
    print("WARNING: soundfile not available")

try:
    import sox
    HAS_SOX = True
except ImportError:
    HAS_SOX = False

# Constants
SUPPORTED_FORMATS = ['.wav', '.mp3', '.flac', '.ogg', '.m4a']
DEFAULT_SAMPLE_RATE = 44100
DEFAULT_CHANNELS = 2


class StemResult:
    """Container for stem separation results."""
    
    def __init__(self):
        self.success = False
        self.input_file = ""
        self.output_dir = ""
        self.stems = {}  # stem_name -> audio_data (float array)
        self.sample_rate = DEFAULT_SAMPLE_RATE
        self.message = ""
        self.error = ""
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "success": self.success,
            "input_file": self.input_file,
            "output_dir": self.output_dir,
            "stems": {k: len(v) for k, v in self.stems.items()},  # store lengths for JSON
            "sample_rate": self.sample_rate,
            "message": self.message,
            "error": self.error
        }


class StemSeparator:
    """Main class for audio stem separation."""
    
    def __init__(self, model: str = "demucs", method: str = "auto"):
        """
        Initialize the stem separator.
        
        Args:
            model: AI model to use ("demucs", "spleeter", "htdemucs")
            method: Separation method ("demucs", "spleeter", "fft", "auto")
        """
        self.model = model
        self.method = method
        self.result = StemResult()
    
    def separate(self, input_path: str, output_dir: str) -> StemResult:
        """
        Separate an audio file into stems.
        
        Args:
            input_path: Path to input audio file
            output_dir: Directory to save stem files
            
        Returns:
            StemResult with separation results
        """
        self.result = StemResult()
        self.result.input_file = input_path
        
        # Validate input file
        if not os.path.exists(input_path):
            self.result.error = f"Input file not found: {input_path}"
            return self.result
        
        # Get file extension
        ext = Path(input_path).suffix.lower()
        if ext not in SUPPORTED_FORMATS:
            self.result.error = f"Unsupported format: {ext}. Supported: {SUPPORTED_FORMATS}"
            return self.result
        
        # Read audio file
        try:
            if HAS_SOUNDFILE:
                audio, sr = sf.read(input_path)
                self.result.sample_rate = sr
            else:
                # Basic fallback - read WAV only
                audio, sr = self._read_wav_fallback(input_path)
                self.result.sample_rate = sr
        except Exception as e:
            self.result.error = f"Failed to read audio file: {e}"
            return self.result
        
        # Ensure mono if stereo (or keep channels)
        if HAS_NUMPY and audio.ndim > 1:
            if audio.shape[1] > 1:
                # Convert to mono by averaging channels
                audio = np.mean(audio, axis=1)
        
        # Try the selected method
        if self.method == "demucs" or (self.method == "auto" and self._check_demucs_available()):
            self._separate_demucs(input_path, output_dir, audio, sr)
        elif self.method == "spleeter" or (self.method == "auto" and self._check_spleeter_available()):
            self._separate_spleeter(input_path, output_dir, audio, sr)
        else:
            self._separate_fft(input_path, output_dir, audio, sr)
        
        return self.result
    
    def _check_demucs_available(self) -> bool:
        """Check if Demucs is available."""
        try:
            result = subprocess.run(
                ["python3", "-c", "import demucs; print('ok')"],
                capture_output=True, text=True, timeout=10
            )
            return result.returncode == 0 and "ok" in result.stdout
        except Exception:
            return False
    
    def _check_spleeter_available(self) -> bool:
        """Check if Spleeter is available."""
        try:
            result = subprocess.run(
                ["python3", "-c", "import spleeter; print('ok')"],
                capture_output=True, text=True, timeout=10
            )
            return result.returncode == 0 and "ok" in result.stdout
        except Exception:
            return False
    
    def _separate_demucs(self, input_path: str, output_dir: str, audio: np.ndarray, sr: int):
        """Separate using Demucs model."""
        try:
            # Create output directory
            os.makedirs(output_dir, exist_ok=True)
            
            # Run Demucs command
            cmd = [
                "python3", "-m", "demucs",
                input_path,
                "-o", output_dir,
                "--device", "cpu",
                "--two-stems", " vocals,noise"  # Can be adjusted
            ]
            
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=300
            )
            
            if result.returncode == 0:
                # Find generated files
                generated_dir = Path(output_dir) / "htdemucs" / Path(input_path).stem
                if generated_dir.exists():
                    self._collect_demucs_stems(generated_dir)
                self.result.success = True
                self.result.message = "Demucs separation completed"
            else:
                self.result.error = f"Demucs failed: {result.stderr[:200]}"
                
        except Exception as e:
            self.result.error = f"Demucs error: {str(e)[:200]}"
    
    def _separate_spleeter(self, input_path: str, output_dir: str, audio: np.ndarray, sr: int):
        """Separate using Spleeter model."""
        try:
            os.makedirs(output_dir, exist_ok=True)
            
            # Spleeter 2stems: drums, vocals
            # Spleeter 4stems: piano, drums, bass, vocals
            cmd = [
                "python3", "-c",
                f"""
import spleeter
from spleeter.separator import Separator
sep = Separator('spleeter:2stems')
audio, sr = ...  # load audio
sep.separate_to_file('{input_path}', '{output_dir}')
                """,
                input_path, output_dir
            ]
            
            # Actually let's use the proper spleeter CLI
            cmd = ["spleeter", "separate", "-i", input_path, "-o", output_dir]
            
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
            
            if result.returncode == 0:
                self.result.success = True
                self.result.message = "Spleeter separation completed"
            else:
                self.result.error = f"Spleeter failed: {result.stderr[:200]}"
                
        except Exception as e:
            self.result.error = f"Spleeter error: {str(e)[:200]}"
    
    def _separate_fft(self, input_path: str, output_dir: str, audio: np.ndarray, sr: int):
        """Basic FFT-based stem separation (fallback method)."""
        try:
            if not HAS_NUMPY:
                self.result.error = "numpy required for FFT separation"
                return
            
            os.makedirs(output_dir, exist_ok=True)
            
            # Simple spectral subtraction approach
            # This is a basic placeholder - real stem separation needs AI
            magnitude = np.abs(np.fft.rfft(audio))
            phase = np.angle(np.fft.rfft(audio))
            
            # Create "clean" stem by filtering high frequencies (simulated engine sound)
            # and "noise" stem by filtering low frequencies
            split_idx = len(magnitude) // 4
            
            # Engine stem: keep lower frequencies
            engine_mag = magnitude.copy()
            engine_mag[split_idx:] = 0
            engine_audio = np.fft.irfft(engine_mag * np.exp(1j * phase))
            
            # Noise stem: keep higher frequencies  
            noise_mag = magnitude.copy()
            noise_mag[:split_idx] = 0
            noise_audio = np.fft.irfft(noise_mag * np.exp(1j * phase))
            
            # Save stems
            engine_path = os.path.join(output_dir, "engine.wav")
            noise_path = os.path.join(output_dir, "noise.wav")
            
            sf.write(engine_path, engine_audio, sr)
            sf.write(noise_path, noise_audio, sr)
            
            self.result.stems = {
                "engine": engine_audio,
                "noise": noise_audio
            }
            self.result.success = True
            self.result.message = "FFT-based separation completed (basic)"
            
        except Exception as e:
            self.result.error = f"FFT separation error: {str(e)[:200]}"
    
    def _read_wav_fallback(self, path: str) -> tuple:
        """Fallback WAV reader using basic parsing."""
        with open(path, 'rb') as f:
            # Read WAV header
            header = f.read(44)
            if header[:4] != b'RIFF':
                raise ValueError("Not a valid WAV file")
            
            sample_rate = struct.unpack('<I', header[24:28])[0]
            channels = struct.unpack('<H', header[22:24])[0]
            bits_per_sample = struct.unpack('<H', header[34:36])[0]
            data_size = struct.unpack('<I', header[40:44])[0]
            
            # Read data
            raw_data = f.read(data_size)
            
            if bits_per_sample == 16:
                samples = np.frombuffer(raw_data, dtype=np.int16).astype(np.float32) / 32768.0
            elif bits_per_sample == 24:
                # Rough 24-bit handling
                samples = np.frombuffer(raw_data, dtype=np.int32).astype(np.float32) / 8388608.0
            else:
                samples = np.frombuffer(raw_data, dtype=np.float32)
            
            # Convert stereo to mono if needed
            if channels == 2 and len(samples) > 0:
                # Assuming interleaved: LRLRLR...
                n = len(samples) // 2
                samples = samples[:2*n].reshape(2, n)
                samples = np.mean(samples, axis=0)
            
            return samples, sample_rate


def main():
    """Main entry point for command-line usage."""
    if len(sys.argv) < 3:
        print("Usage: python stem_separator.py <input_file> <output_dir> [--model demucs|spleeter|htdemucs] [--method demucs|spleeter|fft]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2]
    
    # Parse arguments
    model = "demucs"
    method = "auto"
    
    args = sys.argv[3:]
    i = 0
    while i < len(args):
        if args[i] == "--model" and i + 1 < len(args):
            model = args[i + 1]
            i += 2
        elif args[i] == "--method" and i + 1 < len(args):
            method = args[i + 1]
            i += 2
        else:
            i += 1
    
    separator = StemSeparator(model=model, method=method)
    result = separator.separate(input_file, output_dir)
    
    print(json.dumps(result.to_dict(), indent=2))
    
    if result.success:
        print(f"\nStems separated successfully to: {result.output_dir}")
        for stem_name in result.stems:
            print(f"  - {stem_name}: {len(result.stems[stem_name])} samples at {result.sample_rate}Hz")
    else:
        print(f"\nError: {result.error}")


if __name__ == "__main__":
    main()