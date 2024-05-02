
package Shaders::GL;

@ISA = qw/Shaders/;

sub new { bless {} }

sub PlatformNames { qw/GL GLES/ }

sub Param
{
  $_ = $_[1];

  return '', 0 if $_->{name} eq 'vpos';
  my $vt = $_->{type};
  Shaders::ParamGL($vt);
  my $s = "$_->{vary} $vt $_->{name}";
  $s .= "[$_->{dim}]" if ($_->{dim});
  $s .= (($_->{vary} eq 'uniform') ? ';' : ',') if $_[2];

  return $s, 1;
}

sub Block
{
  my ($this, $stage, $argsr, $b) = @_;
  my %args = %{$argsr};
  local $Shaders::vpos = 'vpos';

  foreach (sort(values %args)) {
    if (lc($_->{sem}) eq 'position') {
      $vpos .= '|\Q'.$_->{name}.'\E';
    }
  }

  $b = Shaders::BlockGL($this, $stage, $argsr, $b);
  if ($this->{name} eq 'GLES') {
    $b =~ s/(^|\W)texture2DLod *\($Shaders::subexpr,$Shaders::subexpr,$Shaders::subexpr\)/\1texture2D(\2,\3)/go;
    $b =~ s/(^|\W)saturate *\($Shaders::subexpr\)/\1clamp(\2,0.0,1.0)/go;
  }
  return $b;
}

sub BeginUniforms { ($_[0]->{stage} eq 'f' && $_[0]->{name} eq 'GLES' ? "precision mediump float;\n" : ''); }

sub Finish
{
  my ($this, $stage, $argsr) = @_;
  my $b = '';
  if ($stage eq 'f') {
    #$b.="  gl_FragColor = fcolor;\n";
  }
  return $b;
}

sub FileExt
{
  return 'glsl';
}

new();
