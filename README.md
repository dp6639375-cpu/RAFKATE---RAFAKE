# WARP – distorsionador multibanda visual (JUCE 8)

weá rara de RAFAKE: curva de transferencia visual + modulación.

## Características
- 3 bandas (crossovers Linkwitz-Riley de 4º orden con compensación de fase)
- 7 algoritmos por banda: Soft, Hard, Fold, Crush, Asym, Sigmoid, Rectify
- Por banda: Drive (0–48 dB), Bias, Mix, Gain, On/Off
- Oversampling 4x, bloqueo de DC, suavizado de parámetros
- Modulación del drive: LFO + seguidor de envolvente
- Filtros HP/LP globales, Mix global, Input/Output
- Curva de transferencia en vivo con punto de señal, y osciloscopio
- Estado/presets del host mediante AudioProcessorValueTreeState
- Formatos: VST3, AU (macOS), Standalone

## Compilar (necesitas CMake ≥ 3.22, Git y un compilador C++17)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Con COPY_PLUGIN_AFTER_BUILD el VST3 se copia a la carpeta estándar del sistema.
- Windows: Visual Studio 2022 (Desktop C++)
- macOS: Xcode command line tools (`xcode-select --install`)
- Linux: instala las dependencias de JUCE (libasound2-dev, libx11-dev, libfreetype-dev, etc.)

## Siguientes pasos para acercarse más a RIFT
Curva editable con puntos arrastrables, más etapas en serie, matriz de modulación, presets de fábrica.
