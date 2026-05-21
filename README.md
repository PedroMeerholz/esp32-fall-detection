# Deep Learning Aplicado à Detecção de Queda

Este repositório contém o firmware e os artefatos usados no projeto de detecção de queda para ESP32.

**Sumário rápido**
- **Pré-requisitos**: Python 3.10+, Git, ESP-IDF (v6.0.1 utilizada neste projeto) e extensão Wokwi
- **Instalação Python**: criar e ativar `venv`, instalar dependências com `pip install -r requirements.txt`
- **Build**: usar `idf.py build`
- **Execução**: via `diagram.json`

**Passo a passo**

**1) Preparar ambiente Python (venv)**

Windows (PowerShell):

```powershell
python -m venv venv
.\\venv\\Scripts\\Activate.ps1
pip install -r requirements.txt
```
Linux / macOS:

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```
**2) Instalar e configurar ESP-IDF (recomendado: v6.0.1)**

Siga as instruções oficiais: https://docs.espressif.com/projects/esp-idf


**3) Ajuste para `esp-tflite-micro` (apenas se usar ESP-IDF v6.0.1)**

Se estiver usando ESP-IDF v6.0.1, edite o arquivo `managed_components/espressif__esp-tflite-micro/CMakeLists.txt` e altere a flag `-O3` para `-O2` na linha que define `common_flags`:

Alterar:

```
set(common_flags -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON -O3
```

para:

```
set(common_flags -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON -O2
```

Isto evita problemas de compatibilidade/otimização com a toolchain usada em v6.0.1.
**4) Build do firmware**

No diretório raiz do projeto (onde está o `CMakeLists.txt` principal):

```bash
cd <repo-root>
idf.py build
```

Se for necessário, selecione o target antes (por exemplo `esp32s2`):

```bash
idf.py set-target esp32s2
idf.py build
```
**5) Flash e monitor**

Conecte a placa e identifique a porta (Windows: `COMx`, Linux: `/dev/ttyUSB0` ou `/dev/ttyACM0`).

```bash
idf.py -p COM3 flash monitor
```

Substitua `COM3` pela sua porta.

**6) Modelos e dados**

Os modelos ficam em `models/`. Certifique-se de que `model_quantized.tflite` (ou o modelo que desejar) esteja presente antes de compilar, se for necessário incluí-lo no firmware.

**7) Limpeza e resolução de problemas**
- Remover diretório `build/` e rodar `idf.py build` novamente para um build limpo.
- Verifique se o ambiente do ESP-IDF está exportado corretamente (`. ./export.sh` ou `.\\export.ps1`).
- Se houver erros relacionados ao TFLite, confirme a alteração de `-O3` para `-O2` descrita acima.

**Referências**
- Documentação ESP-IDF: https://docs.espressif.com
