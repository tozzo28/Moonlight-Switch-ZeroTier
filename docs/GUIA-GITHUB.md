# Publicar manualmente no GitHub

Use este guia para criar um repositório a partir de uma cópia local. Em um clone de
um repositório já publicado, o Git configura `origin` automaticamente; use o fluxo
normal de commits e push do mantenedor. O nome e o email dos commits devem ser os
dados públicos que você escolheu para sua conta.

## 1. Crie um repositório vazio no GitHub

Escolha seu nome de repositório. Evite adicionar README ou licença automática nessa criação,
pois os arquivos corretos já estão nesta pasta.

## 2. Configure a autoria e faça o primeiro commit

Abra um terminal nesta pasta. Configure o nome público e o email que deseja usar nos commits.
Para privacidade, você pode usar o endereço noreply mostrado nas configurações de email do GitHub.
Nenhuma identidade pessoal foi configurada automaticamente nesta entrega.

```bash
git config user.name "SEU_NOME_PUBLICO"
git config user.email "SEU_EMAIL_NOREPLY_DO_GITHUB"
python scripts/check-repository.py
git add .
git status
git commit -m "Prepare Moonlight Switch with embedded ZeroTier"
git remote add origin https://github.com/SEU_USUARIO/SEU_REPOSITORIO.git
git push -u origin main
```

Substitua os três campos em maiúsculas pelos seus dados públicos e pelo endereço do repositório criado.
O GitHub pode pedir que você autentique esse envio. Não coloque tokens dentro dos arquivos ou da URL.

## 3. Publique os binários em Releases

1. Abra a seção Releases e crie uma Release com tag `v1.5.0-zerotier.1` a partir de `main`.
2. Use o texto de `RELEASE_NOTES.md`, revisando o que deseja publicar.
3. Anexe o ZIP do aplicativo, o ZIP completo de fontes e `SHA256SUMS` fornecidos nos artefatos desta entrega.
4. Preserve os avisos de licença e os fontes. Não envie apenas um NRO sem a disponibilização correspondente dos fontes.

Arquivos `.nro`, `.elf`, builds, logs e ZIPs gerados são ignorados pelo Git.
As quatro bibliotecas FFmpeg vendorizadas são uma exceção intencional para permitir a compilação;
seus hashes são verificados. Todos os arquivos preparados para commit estão abaixo do limite
de 100 MiB por arquivo. Os ZIPs grandes pertencem aos anexos da Release, não ao código do repositório.

## Conferência final

Leia `THIRD_PARTY_NOTICES.md` e `docs/LICENCAS-REVISAO.md`.
O pacote foi testado pelo usuário no Switch, mas não há medições detalhadas de desempenho.
Não inclua suas pastas antigas do cartão, a cópia privada de transferência do Ubuntu ou o histórico pessoal.
