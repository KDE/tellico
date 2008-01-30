#!/usr/bin/env ruby
#
# ***************************************************************************
#    copyright            : (C) 2006 by Steve Beattie
#    email                : sbeattie@suse.de
# ***************************************************************************
#
# ***************************************************************************
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of version 2 of the GNU General Public License as  *
# *   published by the Free Software Foundation;                            *
# *                                                                         *
# ***************************************************************************

# $Id: boardgamegeek.rb 313 2006-10-02 22:17:11Z steve $

# This program is expected to be invoked from tellico
# (http://periapsis.org/tellico) as an external data source. It provides
# searches for boardgames from the boardgamegeek.com website, via
# boardgamegeek's xmlapi interface
# (http://www.boardgamegeek.com/xmlapi/)
#
# It only allows searches via name; the boardgamegeek xmlapi is not yet
# rich enough to support queries by designer, publisher, category, or
# mechanism. I'd like to add support for querying by boardgamegeek id,
# but that needs additional support in tellico.
#
require 'rexml/document'
require 'net/http'
require 'cgi'
include REXML

$my_version = '$Rev: 313 $'

class Game
  attr_writer :year
  attr_writer :description

  def initialize(name, id)
    @name = name
    @id = id
    @publishers = []
    @designers = []
    @players = []
  end

  def add_publisher(publisher)
    @publishers << publisher
  end

  def add_designer(designer)
    @designers << designer
  end

  def add_players(players)
    @players << players
  end

  def to_s()
    "@name (#@id #@publishers #@year)"
  end

  def toXML()
    element = Element.new 'entry'
    element.add_element Element.new('title').add_text(@name)
    element.add_element Element.new('description').add_text(@description) if @description
    element.add_element Element.new('year').add_text(@year) if @year
    element.add_element Element.new('boardgamegeek-link').add_text("http://www.boardgamegeek/game/#{@id}") if @id
    element.add_element Element.new('bggid').add_text(@id) if @id
    if @publishers.length > 0
      pub_elements = Element.new('publishers')
      @publishers.each {|p| pub_elements.add_element Element.new('publisher').add_text(p)}
      element.add_element pub_elements
    end
    if @designers.length > 0
      des_elements = Element.new('designers')
      @designers.each {|d| des_elements.add_element Element.new('designer').add_text(d)}
      element.add_element des_elements
    end
    if @players.length > 0
      players_elements = Element.new('num-players')
      @players.each {|n| players_elements.add_element Element.new('num-player').add_text(n.to_s)}
      element.add_element players_elements
    end
    return element
  end
end

def getGameList(query)
  #puts("Query is #{query}")

  search_result = nil
  Net::HTTP.start('www.boardgamegeek.com', 80) do
    |http| search_result = (http.get("/xmlapi/search?search=#{CGI.escape(query)}",
                                    {"User-Agent" => "BoardGameGeek plugin for Tellico #{$my_version}"}).body)
           http.finish
  end
  doc = REXML::Document.new(search_result)

  games = XPath.match(doc, "//game")
  #games.each {|g| puts g.elements['name'].text+g.attributes['gameid']}
  ids = []
  games.each {|g| ids << g.attributes['gameid']}
  return ids
end

def getGameDetails(ids)
  #ids.each {|id| puts id}

  query = "/xmlapi/game/#{ids.join(',')}"
  #puts query
  search_result = nil
  Net::HTTP.start('www.boardgamegeek.com', 80) do
    |http| search_result = (http.get(query, {"User-Agent" => "BoardGameGeek plugin for Tellico #{$my_version}"}).body)
  end
  doc = REXML::Document.new(search_result)

  games_xml = XPath.match(doc, "//game")

  games = []
  games_xml.each do
    |g| game = Game.new(g.elements['name'].text,
                       g.attributes['gameid'])
        game.year = g.elements['yearpublished'].text
        game.description = g.elements['description'].text
        g.elements.each('publisher'){|p| game.add_publisher p.elements['name'].text}
        g.elements.each('designer'){|d| game.add_designer d.elements['name'].text}
        minp = Integer(g.elements['minplayers'].text)
        maxp = Integer(g.elements['maxplayers'].text)
       minp.upto(maxp) {|n| game.add_players(n)}
        games << game
  end
  return games
end

def listToXML(gameList)
  doc = REXML::Document.new
  doc << REXML::DocType.new('tellico PUBLIC', '"-//Robby Stephenson/DTD Tellico V10.0//EN" "http://periapsis.org/tellico/dtd/v10/tellico.dtd"')
  doc << XMLDecl.new
  tellico = Element.new 'tellico'
  tellico.add_attribute('xmlns', 'http://periapsis.org/tellico/')
  tellico.add_attribute('syntaxVersion', '10')
  collection = Element.new 'collection'
  collection.add_attribute('title', 'My Collection')
  collection.add_attribute('type', '13')

  fields = Element.new 'fields'
  field = Element.new 'field'
  field.add_attribute('name', '_default')
  fields.add_element(field)
  field = Element.new 'field'
  field.add_attribute('name', 'bggid')
  field.add_attribute('title', 'BoardGameGeek ID')
  field.add_attribute('category', 'General')
  field.add_attribute('flags', '0')
  field.add_attribute('format', '4')
  field.add_attribute('type', '6')
  field.add_attribute('i18n', 'true')
  fields.add_element(field)
  collection.add_element(fields)

  id = 0
  gameList.each do
    |g| element = g.toXML()
        element.add_attribute('id', id)
        id = id + 1
       collection.add_element(element)
  end
  tellico.add_element(collection)
  doc.add_element(tellico)
  doc.write($stdout, 0)
  puts ""
end

if __FILE__ == $0

  def showUsage
    warn "usage: #{__FILE__} game_query"
    exit 1
  end

  showUsage unless ARGV.length == 1

  idList = getGameList(ARGV.shift)
  if idList
    gameList = getGameDetails(idList)
  end

  listToXML(gameList)
end
