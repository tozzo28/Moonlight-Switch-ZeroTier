# Instalação e conexão

## Requisitos

- Nintendo Switch com ambiente homebrew funcionando e acesso completo à RAM no hbmenu.
- PC com Sunshine funcionando e conectado à internet.
- Uma rede ZeroTier, com o PC e o Switch autorizados nela.

## Instalar o aplicativo

1. Extraia o ZIP do aplicativo disponibilizado na Release.
2. Copie a pasta `switch` para a raiz do cartão.
3. Execute `switch/Moonlight-Switch/Moonlight-Switch.nro` pelo hbmenu.

O pacote não contém perfil, host, identidade ZeroTier, certificado de pareamento ou log de outro usuário.
Ele não apaga configurações já existentes no cartão. Para conferir o primeiro uso limpo,
use uma instalação de teste sem os dados anteriores, preservando seus backups.

## Configurar o ZeroTier

1. Instale o ZeroTier no PC e conecte-o à sua rede virtual.
2. Nas configurações ZeroTier do Moonlight, informe o ID da sua rede e o IPv4 ZeroTier do PC.
3. Habilite ZeroTier, feche completamente o Moonlight e abra-o novamente.
4. Consulte o ID do dispositivo mostrado pelo aplicativo e autorize-o no painel da sua rede ZeroTier.
5. Aguarde a atribuição de um IP. Adicione o PC no Moonlight usando o IP ZeroTier dele.
6. Faça o pareamento pelo PIN no Sunshine e teste um aplicativo ou Desktop.

O PC e o Switch podem usar conexões físicas diferentes. Comece em 720p60 com bitrate moderado
e ajuste conforme a rede. O firewall do PC precisa permitir a comunicação com o Sunshine.

## Dados criados no primeiro uso

O aplicativo pode usar `switch/moonlight`, `switch/Moonlight-Switch` e
`switch/Moonlight-Switch-ZeroTier` para perfis e dados de execução.
Não compartilhe as pastas de identidade ZeroTier ou chaves de pareamento.
Não publique `identity.secret`, arquivos PEM, perfis ou logs sem revisar seu conteúdo.

## Se não conectar

Confira se ambos os dispositivos estão autorizados na mesma rede virtual, se o IP do PC
está correto e se o Sunshine está acessível. O estado online do ZeroTier, isoladamente,
não comprova a disponibilidade do streaming. Mudanças de rede podem exigir fechar e reabrir o aplicativo.
