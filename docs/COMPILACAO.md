# Compilação e testes

## Ambiente validado

- Ubuntu 26.04 via WSL 1; CMake 4.2.3 e GCC de host 15.2.
- devkitA64 r30 / GCC 16.1.0; libnx 4.12.0; switch-tools 1.13.1.
- SDK devkitPro e portlibs do ambiente original. Versões de referência: `SDK-VERSOES.txt`.

As ferramentas Linux do SDK não executam diretamente no Windows. Use Linux ou WSL.
Instale o devkitPro pelas instruções oficiais e configure as portlibs necessárias.
Não há instalador automático do SDK neste repositório.

Dependências de host no Ubuntu:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake python3 pkg-config libcurl4-openssl-dev libexpat1-dev libmbedtls-dev libarchive-tools
export DEVKITPRO=/opt/devkitpro
```

SDK oficial: https://devkitpro.org/wiki/Getting_Started

## Verificar a árvore e executar os testes

```bash
python3 scripts/check-repository.py
bash scripts/test-host.sh
```

Os testes usam tráfego sintético e loopback local; precisam de permissão para abrir sockets.
Eles não conectam à rede ZeroTier de ninguém nem comprovam desempenho no Switch.

## Gerar o NRO

```bash
bash scripts/build-clean-release.sh
```

Esse script executa os testes, recompila ZeroTier Core e curl, compila o frontend moderno
e gera `release/switch/Moonlight-Switch/Moonlight-Switch.nro` com hashes.
Prefere bsdtar quando disponível para contornar a incompatibilidade observada do GNU tar
no Ubuntu 26.04/WSL 1. O arquivo curl upstream tem SHA-256 conferido antes da extração.

As bibliotecas FFmpeg públicas verificadas permanecem pré-compiladas. Seu código-fonte,
configuração registrada e proveniência estão em `third_party/ffmpeg-source/`.
Reproduzir seus bytes não foi parte da validação desta entrega.

## Montar os arquivos de uma Release

Depois de conferir o NRO no aparelho:

```bash
python3 scripts/package-release.py --tag v1.5.0-zerotier.1
```

O empacotador gera ZIP do aplicativo, ZIP de fontes e hashes em `dist/`.
Ele usa os arquivos versionados pelo Git e não inclui `.git`, builds ou perfis ignorados.
Use o NRO gerado a partir dos mesmos fontes que vai distribuir.

O frontend legado é preservado como referência, mas a Release preparada usa o moderno.
