# Tarefa 06 – Execução de Modelo TensorFlow Lite Micro em SoC LiteX

Este projeto implementa a execução de um modelo **TensorFlow Lite Micro (TFLM)** no processador **VexRiscv** de um **SoC LiteX** sintetizado na **FPGA ColorLight i9**.  
O modelo utilizado é o **hello_world**, que gera valores aproximados de uma função seno. Esses valores são usados para controlar **8 LEDs externos**, criando um efeito “barra de luz” proporcional e com animação de ida-e-volta.

---

### Objetivo da Tarefa

- Portar o TensorFlow Lite Micro para o LiteX + VexRiscv.
- Executar o modelo *hello_world* em **bare-metal**.
- Usar a saída do modelo para **controlar LEDs externos via GPIO mapeada em CSR**.
- Implementar efeito visual proporcional ao valor inferido.
- Fazer o sistema operar **totalmente autônomo após boot**.

---

### Organização do Projeto

O projeto está organizado com os seguintes diretórios principais:

-   `firmware/`: Contém os arquivos de integração com o firmware (`main.c`, `Makefile`).
-   `litex/`: Reúne os arquivos de integração com o LiteX
-   Makefile's
-   hello_world_tflite_micro_fix.ipynb

---

### Mapeamento de Hardware

### LEDs Externos – Placa de Expansão

8 LEDs conectados via cabo flat IDC 14 pinos.

| Bit | LED | Pino FPGA(CN3)| CSR |
|-----|-----|-----------|------------|
| 0   | L1  | E19        | leds_ext[0] |
| 1   | L2  | B3        | leds_ext[1] |
| 2   | L3  | K4        | leds_ext[2] |
| 3   | L4  | A2       | leds_ext[3] |
| 4   | L5  | J5        | leds_ext[4] |
| 5   | L6  | J4        | leds_ext[5] |
| 6   | L7  | K3       | leds_ext[6] |
| 7   | L8  | K5       | leds_ext[7] |

O pino 1 do cabo IDC é marcado pela faixa vermelha.

---

## Toolchain
A toolchain utilizada é a [OSS CAD Suite](https://github.com/YosysHQ/oss-cad-suite-build), que integra binários de todas as etapas necessárias:
- Síntese RTL: [Yosys](https://github.com/YosysHQ/yosys);
- Place and Route: [nextpnr](https://github.com/YosysHQ/nextpnr) e [Project Trellis](https://github.com/YosysHQ/prjtrellis);
- Gravação da FPGA: [ECPDAP](https://github.com/adamgreig/ecpdap) ou [openFPGALoader](https://github.com/trabucayre/openFPGALoader)

 O projeto também utiliza o framework do [LiteX](https://github.com/enjoy-digital/litex) e a toolchain bare-metal do RISC-V: [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain/releases).

---

## Instalação do LiteX

O LiteX é uma ferramenta em Python. O uso dentro de um ambiente virtual (*venv*) é recomendado.
Como a OSS CAD Suite já cria seu próprio ambiente Python, o ideal é ativar o ambiente da OSS CAD Suite antes de instalar o LiteX.

### Passos de Instalação

1. Crie uma pasta para o LiteX:
   ```bash
   mkdir -p litex
   cd litex
   ```

2. Baixe o script de setup:
   ```bash
   wget https://raw.githubusercontent.com/enjoy-digital/litex/master/litex_setup.py
   ```

3. Execute a instalação completa (dentro do ambiente OSS CAD Suite):
   ```bash
   python3 litex_setup.py --init --install --config=full
   ```

4. Instale dependências adicionais:
   ```bash
   pip3 install meson ninja
   ```

5. Instale a toolchain bare-metal RISC-V:
   - [riscv-gnu-toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain/releases)
   - O compilador `riscv32-unknown-elf-gcc` deve estar no PATH.

Após a instalação, sempre que o ambiente OSS CAD Suite estiver ativo, o LiteX também estará disponível.

---

## Build do SoC

O script `litex/colorlight_i5.py` gera e sintetiza o SoC que inclui o acelerador de produto escalar.

Para gerar o bitstream:

```bash
$(which python3) litex/colorlight_i5.py --board i9 --revision 7.2 --build --ecppack-compress
```

O bitstream resultante será salvo em:

```
build/colorlight_i5/gateware/colorlight_i5.bit
```

---

## Gravação na FPGA

Para programar a FPGA Colorlight i5 com o bitstream gerado:

```bash
$(which openFPGALoader) -b colorlight-i5 build/colorlight_i5/gateware/colorlight_i5.bit
```

---

## Execução do Firmware

Com a FPGA já configurada com o bitstream, compile e embarque o firmware LiteX:

```bash
cd firmware/
make clean
make
litex_term /dev/ttyACM0 --kernel main.bin
```
(O caminho  `/dev/ttyACM0` deve apontar para o dispositivo serial (porta de comunicação) que o Linux cria)

dentro do terminal: litex> utilize o comando: `reboot` para inicializar com seu firmware.

**Durante a execução, o firmware:**

- Inicializa o modelo TensorFlow Lite Micro.
- Realiza um teste rápido dos LEDs via GPIO.
- Executa inferências contínuas do modelo *hello_world* (função seno).
- Converte a saída da inferência em um padrão de LEDs proporcional ao valor previsto.
- Exibe no console o valor de entrada, a saída do modelo e o estado dos LEDs.
- Reinicia o ciclo ao atingir o valor equivalente a 2 * pi.
- Permite encerrar a execução com **Ctrl+P**, apagando todos os LEDs.

