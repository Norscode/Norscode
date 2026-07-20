# Startapp

`nc startapp` lagar ein ny app-modul i eit eksisterande prosjekt.

Bruk:

- `nc startapp users`
- `nc startapp users --path /sti/til/prosjekt`

Genererte filer:

- `apps/users/users.no`
- `apps/users/README.md`
- `apps/users/tests/test_users.no`

Sørg for at du køyrer kommandoen frå prosjektrot eller peikar `--path` mot prosjektrot med `norcode.toml`.
