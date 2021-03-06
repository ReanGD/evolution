#include "scene.h"
#include "settings.h"

static const Mass baseOrgamismMass = Mass(100);
static const Mass baseFoodMass = Mass(10);
static const uint32_t foodCnt = 400;
static const uint32_t maxOrgamismCnt = 150;

Cell::Cell(CellType type, Mass mass)
  : m_type(type)
  , m_mass(mass) {

}

void Cell::move(Cell& to) {
  to.m_type = m_type;
  m_type = CellType::Space;

  to.m_mass += m_mass;
  m_mass = Mass(0);
}

Organism::Organism(Position position, Generator& generator)
  : m_died(false)
  , m_position(position)
  //, m_direction(generator.get(Direction::Last))
  , m_direction(Direction::Forward)
//  , m_network(static_cast<size_t>(Direction::Last) * static_cast<size_t>(CellType::Last), static_cast<size_t>(Direction::Last) * static_cast<size_t>(CellType::Last), 8, generator) {
  , m_network(static_cast<size_t>(Direction::Last), static_cast<size_t>(Direction::Last), 8, generator) {
}

void Organism::cloneFrom(Organism *parent, Position position, Generator &generator) {
  m_died = false;
  m_position = position;
  m_direction = generator.get(Direction::Last);
  m_direction = Direction::Forward;
  m_network.cloneFrom(parent->m_network, generator);
}

void Organism::calc(World* word, Cell& currentCell, const std::array<Cell*, static_cast<size_t>(Direction::Last)>& cells) {

//  auto inputs = std::shared_ptr<float[]>(new float[static_cast<size_t>(Direction::Last) * static_cast<size_t>(CellType::Last)]);
  auto inputs = std::shared_ptr<float[]>(new float[static_cast<size_t>(Direction::Last)]);
  size_t i=0;
  for (const auto cell :cells) {
//    for(uint32_t cellType=0; cellType!=static_cast<uint32_t>(CellType::Last); ++cellType) {
//      if (static_cast<uint32_t>(cell->m_type) == cellType) {
//        inputs[i++] = 1.0f;
//      } else {
//        inputs[i++] = 0.0f;
//      }
//    }
          if (cell->m_type == CellType::Food) {
            inputs[i++] = 1.0f;
          } else {
            inputs[i++] = 0.0f;
          }

  }

  std::shared_ptr<float[]> result = m_network.calc(inputs);
  Direction directionMove = Direction::Left;
  float maxValue = 0.0f;
  for (uint32_t i=0; i!=static_cast<uint32_t>(Direction::Last); ++i) {
    if (result.get()[i] > maxValue) {
      maxValue = result.get()[i];
      directionMove = static_cast<Direction>(i);
    }
  }

  Position newPosition = m_position;
  newPosition.move(directionMove + m_direction);
  currentCell.m_mass -= Mass(1);

  if (m_position != newPosition) {
    Cell& newCell = word->getCell(newPosition);
    CellType toType = newCell.m_type;

    if ((toType == CellType::Space) || (toType == CellType::Food)) {
      m_position = newPosition;
      currentCell.move(newCell);
    }

    if (toType == CellType::Food) {
      word->addFood();
    }
  }

  if (word->getCell(m_position).m_mass.m_value == 0) {
    word->getCell(m_position).m_type = CellType::Space;
    m_died = true;
  }

}

World::World()
  : m_cells(Settings::m_worldMaxCoord, Cell(CellType::Space, Mass(0))) {

  m_cntOrganisms = maxOrgamismCnt;
  initOrganisms(maxOrgamismCnt);

  for(size_t i=0; i!=foodCnt; ++i) {
    addFood();
  }
}

void World::addFood() {
  Position coord = getRandomSpaceCell();
  m_cells[coord] = Cell(CellType::Food, baseFoodMass);
}

void World::step() {
  for (auto& organism: m_organisms) {
    if (organism->isDied()) {
      continue;
    }
    Position pos = organism->getPosition();
    Direction dir = organism->getDirection();

    Cell barrier(CellType::Barrier, Mass(0));
    std::array<Cell*, static_cast<size_t>(Direction::Last)> cells;
    for(size_t i=0; i!=cells.size(); ++i) {
      Position copyPos = pos;
      copyPos.move(static_cast<Direction>(i) + dir);
      if (copyPos == pos) {
        cells[i] = &barrier;
      } else {
        cells[i] = &m_cells[copyPos];
      }
    }
    organism->calc(this, m_cells[pos], cells);
    if (organism->isDied()) {
      m_cntOrganisms--;
    }

    if (m_cntOrganisms == 10) {
      reinitOrganisms();
      return;
    }
  }

  m_currentGenerationLife++;
}

Position World::getRandomSpaceCell() const {
  while(true) {
    auto coord = m_random.get(Settings::m_worldMaxCoord);
    if (m_cells[coord].m_type == CellType::Space) {
      return Position(coord);
    }
  }
}

void World::initOrganisms(uint32_t cnt) {
  for(size_t i=0; i!=cnt; ++i) {
    Position coord = getRandomSpaceCell();
    m_organisms.push_front(std::make_unique<Organism>(coord, m_random));
    m_cells[coord] = Cell(CellType::Organism, baseOrgamismMass);
  }
}

void World::reinitOrganisms() {
  m_generation++;
  m_lastGenerationLife = m_currentGenerationLife;
  m_currentGenerationLife = 0;
  std::array<Organism*, 10> survivors;
  uint32_t i=0;
  for (auto& organism: m_organisms) {
    if (!organism->isDied()) {
      survivors[i++] = organism.get();
    }
  }

  i=0;
  for (auto& organism: m_organisms) {
    if (organism->isDied()) {
      Organism* parent = survivors[i % 10];
      i++;
      Position coord = getRandomSpaceCell();
      organism->cloneFrom(parent, coord, m_random);
      m_cells[coord] = Cell(CellType::Organism, baseOrgamismMass);
      m_cntOrganisms++;
    }
  }
}
