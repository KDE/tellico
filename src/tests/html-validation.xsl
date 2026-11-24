<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-2.0-or-later -->
<!-- Semantic checks for Tellico-generated HTML, parsed by libxml2.
     The first output line is a sentinel; subsequent lines are violations. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text" encoding="UTF-8"/>
  <xsl:key name="ids" match="*[@id]" use="@id"/>
  <xsl:key name="anchors" match="a[@name]" use="@name"/>

  <xsl:template match="/">
    <xsl:text>HTML validation&#10;</xsl:text>
    <xsl:apply-templates select="//*[@id]" mode="id"/>
    <xsl:apply-templates select="//img" mode="image"/>
    <xsl:apply-templates select="//a[@href]" mode="link"/>
    <xsl:apply-templates select="//ul/* | //ol/* | //menu/*" mode="list"/>
    <xsl:apply-templates select="//td[@colspan or @rowspan] | //th[@colspan or @rowspan]" mode="span"/>
    <xsl:apply-templates select="//table" mode="table"/>
    <xsl:apply-templates select="//*[@style]" mode="style"/>
    <xsl:apply-templates select="//*[@width or @height or @size or @span or @start or @value or @colspan or @rowspan or @border or @cellpadding or @cellspacing]" mode="numeric"/>
    <xsl:apply-templates select="//td | //th | //tr | //thead | //tbody | //tfoot" mode="table-parent"/>
    <xsl:apply-templates select="//td[@headers] | //th[@headers]" mode="headers"/>
    <xsl:apply-templates select="//*[local-name() = 'i18n' or starts-with(name(), 'tc:') or starts-with(name(), 'xsl:') or namespace-uri() = 'http://periapsis.org/tellico/' or namespace-uri() = 'http://www.w3.org/1999/XSL/Transform']" mode="unprocessed"/>
  </xsl:template>

  <xsl:template name="error">
    <xsl:param name="message"/>
    <xsl:value-of select="$message"/>
    <xsl:text> [</xsl:text>
    <xsl:for-each select="ancestor-or-self::*">
      <xsl:variable name="tag" select="name()"/>
      <xsl:text>/</xsl:text><xsl:value-of select="$tag"/>
      <xsl:text>[</xsl:text><xsl:value-of select="count(preceding-sibling::*[name() = $tag]) + 1"/><xsl:text>]</xsl:text>
    </xsl:for-each>
    <xsl:text>]&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="*" mode="id">
    <xsl:if test="not(string(@id)) or contains(@id, ' ') or contains(@id, '&#9;') or contains(@id, '&#10;') or contains(@id, '&#13;')">
      <xsl:call-template name="error"><xsl:with-param name="message" select="'Invalid ID'"/></xsl:call-template>
    </xsl:if>
    <xsl:if test="count(key('ids', @id)) &gt; 1 and generate-id() = generate-id(key('ids', @id)[1])">
      <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Duplicate ID: ', @id)"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="img" mode="image">
    <xsl:if test="not(normalize-space(@src)) and not(normalize-space(@srcset))">
      <xsl:call-template name="error"><xsl:with-param name="message" select="'Image without source'"/></xsl:call-template>
    </xsl:if>
    <xsl:for-each select="@width | @height">
      <xsl:variable name="value" select="normalize-space(.)"/>
      <!-- Allow legacy HTML percentage dimensions. -->
      <xsl:variable name="digits" select="substring($value, 1, string-length($value) - number(substring($value, string-length($value)) = '%'))"/>
      <xsl:if test="not(number($digits) &gt; 0) or translate($digits, '0123456789', '') != ''">
        <xsl:variable name="message" select="concat('Invalid image ', name(), ': ', .)"/>
        <xsl:for-each select=".."><xsl:call-template name="error"><xsl:with-param name="message" select="$message"/></xsl:call-template></xsl:for-each>
      </xsl:if>
    </xsl:for-each>
    <xsl:if test="ancestor::a[@href and not(normalize-space(@href))]">
      <xsl:call-template name="error"><xsl:with-param name="message" select="'Empty image hyperlink'"/></xsl:call-template>
    </xsl:if>
    <xsl:if test="not(@alt)">
      <xsl:call-template name="error"><xsl:with-param name="message" select="'Image without alt attribute'"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="a" mode="link">
    <xsl:variable name="fragment" select="substring(@href, 2)"/>
    <!-- Ignore encoded fragments, text fragments, and the special #top target. -->
    <xsl:if test="starts-with(@href, '#') and string($fragment) and not(contains($fragment, '%')) and not(contains($fragment, ':~:')) and translate($fragment, 'TOP', 'top') != 'top' and not(key('ids', $fragment) or key('anchors', $fragment))">
      <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Unresolved anchor: ', @href)"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="*" mode="list">
    <xsl:if test="not(self::li or self::script or self::template)">
      <xsl:call-template name="error"><xsl:with-param name="message" select="'Invalid list child'"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="td|th" mode="span">
    <xsl:for-each select="@colspan | @rowspan">
      <xsl:variable name="value" select="normalize-space(.)"/>
      <xsl:if test="not(string($value)) or translate($value, '0123456789', '') != '' or (name() = 'colspan' and (number($value) &lt; 1 or number($value) &gt; 1000)) or (name() = 'rowspan' and number($value) &gt; 65534)">
        <xsl:variable name="message" select="concat('Invalid ', name(), ': ', .)"/>
        <xsl:for-each select="..">
          <xsl:call-template name="error"><xsl:with-param name="message" select="$message"/></xsl:call-template>
        </xsl:for-each>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="table" mode="table">
    <xsl:variable name="rows" select="tr | thead/tr | tbody/tr | tfoot/tr"/>
    <xsl:variable name="cells" select="$rows/td | $rows/th"/>
    <!-- Width checking is a Tellico quality rule, not a full HTML grid solver.
         Skip vertical spans and malformed colspan values; nested tables are independent. -->
    <xsl:if test="not($cells[@rowspan and not(number(@rowspan) = 1)]) and not($cells[@colspan and (not(number(@colspan) &gt;= 1) or number(@colspan) &gt; 1000 or translate(normalize-space(@colspan), '0123456789', '') != '')])">
      <xsl:variable name="first" select="$rows[1]/*[self::td or self::th]"/>
      <xsl:variable name="width" select="count($first[not(@colspan)]) + sum($first/@colspan)"/>
      <xsl:for-each select="$rows">
        <xsl:variable name="row" select="td | th"/>
        <xsl:variable name="actual" select="count($row[not(@colspan)]) + sum($row/@colspan)"/>
        <xsl:choose>
          <xsl:when test="not($row)">
            <xsl:call-template name="error"><xsl:with-param name="message" select="'Empty table row'"/></xsl:call-template>
          </xsl:when>
          <xsl:when test="$actual != $width">
            <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Inconsistent table width: expected ', $width, ', found ', $actual)"/></xsl:call-template>
          </xsl:when>
        </xsl:choose>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <xsl:template match="*" mode="numeric">
    <xsl:for-each select="@width | @height | @size | @span | @start | @colspan | @rowspan | @border | @cellpadding | @cellspacing | @value[parent::li or parent::meter or parent::progress]">
      <xsl:if test="contains(translate(., 'NAN', 'nan'), 'nan')">
        <xsl:variable name="message" select="concat('NaN in numeric attribute: ', name(), '=', .)"/>
        <xsl:for-each select=".."><xsl:call-template name="error"><xsl:with-param name="message" select="$message"/></xsl:call-template></xsl:for-each>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="*" mode="style">
    <!-- Only split simple declarations. Quoted strings, functions, comments,
         and escapes can contain semicolons; leave these to a CSS parser. -->
    <xsl:if test="not(contains(@style, '&quot;') or contains(@style, &quot;'&quot;) or contains(@style, '(') or contains(@style, '/*') or contains(@style, '\'))">
      <xsl:call-template name="css-declarations"><xsl:with-param name="rest" select="@style"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="css-declarations">
    <xsl:param name="rest"/>
    <xsl:variable name="declaration" select="normalize-space(substring-before(concat($rest, ';'), ';'))"/>
    <xsl:variable name="property" select="normalize-space(substring-before($declaration, ':'))"/>
    <xsl:variable name="value" select="normalize-space(substring-after($declaration, ':'))"/>
    <xsl:if test="string($declaration) and (not($property) or not($value) or contains($property, '=') or contains($property, ' '))">
      <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Malformed inline CSS: ', $declaration)"/></xsl:call-template>
    </xsl:if>
    <xsl:if test="($property = 'width' or $property = 'height' or $property = 'min-width' or $property = 'max-width' or $property = 'min-height' or $property = 'max-height' or $property = 'font-size' or $property = 'line-height') and starts-with(translate($value, 'NAN', 'nan'), 'nan')">
      <xsl:call-template name="error"><xsl:with-param name="message" select="concat('NaN in inline CSS: ', $declaration)"/></xsl:call-template>
    </xsl:if>
    <xsl:if test="($property = 'align-items' or $property = 'align-self') and ($value = 'top' or $value = 'bottom')">
      <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Invalid CSS alignment: ', $declaration)"/></xsl:call-template>
    </xsl:if>
    <xsl:if test="contains($rest, ';')"><xsl:call-template name="css-declarations"><xsl:with-param name="rest" select="substring-after($rest, ';')"/></xsl:call-template></xsl:if>
  </xsl:template>

  <xsl:template match="*" mode="table-parent">
    <xsl:if test="((self::td or self::th) and not(parent::tr)) or (self::tr and not(parent::table or parent::thead or parent::tbody or parent::tfoot)) or ((self::thead or self::tbody or self::tfoot) and not(parent::table))">
      <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Orphaned table element: ', name())"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="td|th" mode="headers">
    <xsl:call-template name="header-tokens"><xsl:with-param name="rest" select="normalize-space(@headers)"/></xsl:call-template>
  </xsl:template>

  <xsl:template name="header-tokens">
    <xsl:param name="rest"/>
    <xsl:if test="string($rest)">
      <xsl:variable name="token" select="substring-before(concat($rest, ' '), ' ')"/>
      <xsl:variable name="table" select="ancestor::table[1]"/>
      <xsl:variable name="cell" select="."/>
      <xsl:if test="not($table) or not(key('ids', $token)[self::th and generate-id(ancestor::table[1]) = generate-id($table) and generate-id() != generate-id($cell)])">
        <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Unresolved table header: ', $token)"/></xsl:call-template>
      </xsl:if>
      <xsl:call-template name="header-tokens"><xsl:with-param name="rest" select="substring-after($rest, ' ')"/></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="*" mode="unprocessed">
    <xsl:call-template name="error"><xsl:with-param name="message" select="concat('Unprocessed markup: ', name())"/></xsl:call-template>
  </xsl:template>
</xsl:stylesheet>
