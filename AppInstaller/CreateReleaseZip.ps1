cd $args[0]

if (Test-Path -Path $args[1])
{
	rm -fo $args[1]
}

Compress-Archive ./* $args[1]