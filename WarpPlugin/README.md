# WARP – distorsión bipolar multibanda (JUCE 8)

Plugin creativo de distorsión inspirado en el *concepto* de RIFT (Minimal Audio). Código y diseño propios;
no usa código, algoritmos ni recursos de Minimal Audio, y no es una réplica idéntica.

## Funciones
- **Motor bipolar:** algoritmo distinto para la mitad positiva y la negativa de la onda, por banda
- **30 algoritmos** de waveshaping (saturación, folds, bitcrush, chebyshev, fuzz, diodo, glitch…)
- **3 bandas** (Linkwitz-Riley 4º orden, compensación de fase): Drive 0–48 dB, Bias, Mix, Gain, On/Off
- **Oversampling 4x**, bloqueo de DC, parámetros suavizados
- **24 tipos de filtro** (pre y post): básicos (LP/HP 12-24, BP, notch, allpass, peak), morph, tilt, formant,
  shelves, peaks dobles, notch doble, scoop y 6 combs armónicos
- **Feedback afinado** con pitch snap a semitonos y seguimiento MIDI, damping y mix
- **Modulación:** LFO, editor de curva de 8 puntos, Follow (envolvente) y 2 macros → Drive / Cutoff / Feedback
- **327 presets** (7 hechos a mano + 320 generados proceduralmente en 8 familias: Subtle, Bass, Lead, Drums,
  Rhythmic, Resonant, Extreme, Lo-Fi), menú por categorías, botones anterior/siguiente y **Randomize**
- **Vista Play** (botón "Play"): solo los 2 macros grandes + Input/Output/Mix
- **Scope multibanda:** espectro con las 3 bandas y los crossovers (clic para cambiar a osciloscopio)
- **Modo claro/oscuro** (se guarda con el proyecto)
- Formatos: VST3, AU (macOS), Standalone

## Compilar (CMake ≥ 3.22, Git, compilador C++17)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

- Windows: Visual Studio 2022 (Desktop C++)
- macOS: `xcode-select --install`
- Linux: dependencias de JUCE (libasound2-dev, libx11-dev, libfreetype-dev, libcurl4-openssl-dev, etc.)

## Notas
- Código sin compilar en el entorno de creación: si hay errores en la primera build, repórtalos.
- Los presets generados son variaciones aleatorias con semilla fija: conviene revisarlos y quedarse con los mejores.
- El tema claro/oscuro es global por proceso (varias instancias en el mismo DAW comparten la apariencia mientras están abiertas).
- Para "MIDI track", el DAW debe enviar notas MIDI al plugin.
