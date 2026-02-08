# fffb

### plugin de force feedback para macOS

> fork do [eddieavd/fffb](https://github.com/eddieavd/fffb) — muito obrigado ao projeto original por tornar o force feedback no macOS possível

## o que?

### fffb

`fffb` é um plugin para [jogos da scs](https://www.scssoft.com/) que usa o [TelemetrySDK](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry) para ler dados de telemetria do caminhão, construir forças customizadas e reproduzi-las no seu volante logitech

## por que?

- o logitech GHUB é uma bela porcaria
- force feedback não funciona no macOS
- eu gosto de como o volante treme quando piso no acelerador

## como?

a parte do sdk da scs foi adaptada dos exemplos incluídos no TelemetrySDK.
a comunicação com o volante é feita pela API de dispositivos HID da apple e usa o protocolo clássico de FFB da logitech.

## funcionalidades

### modo nativo de 900 graus

o plugin configura automaticamente o volante para o modo nativo de 900 graus de rotação durante a inicialização, proporcionando o range completo de rotação para simulação de caminhões.

### efeitos de force feedback

o `fffb` utiliza todos os 4 slots de efeito de força do hardware para proporcionar imersão realista:

| slot            | efeito                 | descrição                                                                                                                                                                                                          |
| --------------- | ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0 (constante)   | torque auto-alinhante  | usa a telemetria de aceleração lateral para puxar o volante de volta ao centro nas curvas. mais forte em velocidade, reduzido em frenagem forte para simular perda de aderência                                    |
| 1 (mola)        | mola de centralização  | força de centralização dependente de velocidade com progressão gradual. leve em manobras de estacionamento, forte na estrada. pequena zona morta no centro para não brigar com o jogador                           |
| 2 (amortecedor) | resistência da direção | amortecimento influenciado por velocidade e freio. proporciona peso realista na direção que aumenta com a velocidade. frenagem adiciona resistência extra para simular transferência de peso para o eixo dianteiro |
| 3 (trapezoide)  | textura da superfície  | ativa em superfícies off-road (cascalho, terra). a intensidade da oscilação aumenta com a velocidade e responde a mudanças na deflexão da suspensão para detecção de lombadas                                      |

### canais de telemetria

o plugin lê os seguintes dados de telemetria do jogo:

- velocidade, RPM, marcha do caminhão
- direção, acelerador, freio, embreagem efetivos
- orientação do caminhão (direção, inclinação, rolagem)
- **aceleração lateral** — alimenta o efeito de torque auto-alinhante
- **deflexão da suspensão das rodas dianteiras** (esquerda + direita) — alimenta a detecção de lombadas para efeitos de superfície
- **substância da superfície das rodas** (esquerda + direita) — detecta superfícies off-road

### LEDs de RPM

os LEDs indicadores de RPM do volante são controlados pela telemetria de RPM do motor, acendendo progressivamente conforme o RPM aumenta.

## volantes suportados

- Logitech G29 (PS4)
- Logitech G923 (PS)
- outros volantes Logitech com protocolo clássico de FFB podem funcionar (não testados)

## uso

### binários pré-compilados

binários estão disponíveis na [página de releases](https://github.com/marcosgugs/fffb/releases)
basta copiar `libfffb.dylib` para o diretório de plugins
(o diretório de plugins deve estar ao lado do executável do jogo, o caminho padrão para o ats seria `~/Library/Application\ Support/Steam/steamapps/common/Euro\ Truck\ Simulator 2/Euro\ Truck\ Simulator 2.app/Contents/MacOS/plugins`)

### compilando a partir do código fonte

para compilar o `fffb`:

```bash
# clone o repositório
git clone https://github.com/marcosgugs/fffb && cd fffb

# crie o diretório de build
mkdir build && cd build

# configure e compile o projeto
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# crie o diretório de plugins
## deve estar no mesmo diretório que o executável do ets2/ats
## este deve ser o caminho padrão
mkdir ~/Library/Application\ Support/Steam/steamapps/common/Euro\ Truck\ Simulator 2/Euro\ Truck\ Simulator 2.app/Contents/MacOS/plugins

# copie o plugin para o diretório de plugins
cp libfffb.dylib ~/Library/Application\ Support/Steam/steamapps/common/Euro\ Truck\ Simulator 2/Euro\ Truck\ Simulator 2.app/Contents/MacOS/plugins
```

alternativamente, você pode usar o script de build para limpar, compilar e instalar em um único passo:

```bash
# compilar e instalar para Euro Truck Simulator 2 (padrão)
./build_and_install.sh

# ou explicitamente
./build_and_install.sh --ets

# compilar e instalar para American Truck Simulator
./build_and_install.sh --ats
```

agora você pode iniciar o ets2/ats.
ao iniciar, o volante deve fazer uma calibração e você verá o popup de funcionalidades avançadas do sdk, clique em OK.
se o volante começar a girar (de forma similar a quando é conectado), a inicialização foi bem sucedida e você está pronto!
se o volante não girar, você pode tentar recarregar o plugin executando `sdk reinit` no console do jogo.
caso isso também não ajude, sinta-se à vontade para abrir uma issue e incluir o arquivo de log do `fffb` localizado em `/tmp/fffb.log`

## solução de problemas

- **volante não calibra ao iniciar**: tente `sdk reinit` no console do jogo
- **forças muito fracas/fortes**: as constantes de ajuste de força estão em `include/fffb/force/simulator.hxx` — ajuste os valores de ganho em `_update_constant` e as curvas de amplitude em `_update_spring`
- **verificar logs**: os logs do plugin são gravados em `/tmp/fffb.log`

## aviso

deve funcionar em qualquer mac com apple silicon
como a scs exige que os binários sejam x86_64, pode funcionar diretamente em macs x86_64 mais antigos (não testado até o momento)
funciona apenas com volantes Logitech
sou apenas um cara aleatório escrevendo código porque está irritado com a logitech, use por sua conta e risco
